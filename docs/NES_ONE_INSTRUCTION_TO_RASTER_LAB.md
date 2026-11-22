# One instruction, one raster dot, an entire console

*A first-principles C++23 source laboratory following a cartridge byte through the CPU, bus, PPU and host framebuffer.*

An NES program does not say “draw a sprite” to the operating system. It reads and writes addresses. A cartridge supplies program and picture-pattern bytes; the 6502 interprets some as instructions; the PPU fetches others as tiles, palettes and sprites. KairoNES exposes these interactions in [`Cartridge.cpp`](../NESEmulator/Cartridge.cpp), [`Mapper_000.cpp`](../NESEmulator/Mapper_000.cpp), [`Bus.cpp`](../NESEmulator/Bus.cpp), [`cpu6502.cpp`](../NESEmulator/cpu6502.cpp), [`ppu2C02.cpp`](../NESEmulator/ppu2C02.cpp), [`Mapper_004.cpp`](../NESEmulator/Mapper_004.cpp) and [`main.cpp`](../NESEmulator/main.cpp). Read those in order alongside this chapter.

The existing emulator is educational; instruction-level timing is not exact per-cycle 6502 bus operation timing, the APU is incomplete, and MMC3 IRQ counting is simplified. No CPU comments or static source trace certify hardware parity.

## 1. One number, three address spaces

Suppose a single-bank NROM cartridge has 16 KiB of PRG ROM. The CPU asks to read `$C000`. It does **not** directly index `vPRGMemory[0xC000]`.

```text
CPU::read($C000)
    -> Bus::cpuRead($C000)
    -> Cartridge::cpuRead($C000)
    -> Mapper_000::cpuMapRead($C000)
    -> mapped offset = $C000 & $3FFF = $0000
    -> Cartridge PRG bytes[0]     (only after bounds check)
```

NROM maps its one 16 KiB PRG bank twice: CPU `$8000–$BFFF` and `$C000–$FFFF` refer to the same offsets `$0000–$3FFF`. In a two-bank 32 KiB NROM image, the mask changes to `$7FFF` and `$C000` addresses offset `$4000`. The PPU's `$0000–$1FFF` CHR pattern space is a **different address bus**, not these CPU addresses.

The new [iNES input lab](NES_CARTRIDGE_TRUST_BOUNDARY_AND_CONTROLLER_IO_LAB.md) explains why the loader validates ROM size and mapper metadata before any offset is used.

## 2. The reset vector starts the actual program

The CPU's [`reset()`](../NESEmulator/cpu6502.cpp) reads two *little-endian* bytes through the bus from CPU `$FFFC` and `$FFFD`. On one-bank NROM these map to PRG offsets `$3FFC` and `$3FFD`.

Suppose the bytes are `$00` and `$80`. Then:

```text
PC = lowByte | (highByte << 8)
   = $00 | ($80 << 8)
   = $8000
```

The CPU also sets its stack pointer to `$FD`, clears A/X/Y, sets the unused status bit and initializes an eight-cycle reset delay. No ROM file parser is involved in this instruction phase; the mapper already owns the translation from CPU address to program bytes.

## 3. An opcode and its addressing mode are separate jobs

Imagine these bytes at CPU address `$8000`:

| Address | Byte | Meaning |
| --- | --- | --- |
| `$8000` | `$A9` | LDA immediate |
| `$8001` | `$7F` | Operand +127 |
| `$8002` | `$69` | ADC immediate |
| `$8003` | `$01` | Operand +1 |

When `cpu6502::clock()` has no pending cycles, it reads the opcode via the Bus, increments PC, looks up `lookup[opcode]`, invokes the *addressing-mode callback* and then the *operation callback*. For LDA immediate, `IMM()` establishes where the operand lives and `LDA()` takes it into A through `fetch()`. For ADC immediate, the addressing mode is reused but `ADC()` performs different arithmetic.

This division of labor explains the line `cycles += (additional_cycle1 & additional_cycle2)`: an indexed addressing mode may signal a page crossing, but only an operation eligible for that penalty asks for the extra cycle. The code changes architectural state at the **beginning** of the instruction's counted cycles, then counts the delay; a physical 6502 performs observable micro-operations across those cycles. That distinction affects DMA and PPU register timing.

## 4. Carry and overflow tell two different stories

After `LDA #$7F`, A holds `$7F`. Assume Carry=0. The next `ADC #$01` computes a 16-bit temporary:

```text
$7F + $01 + 0 = $0080

new accumulator A = $80
Carry C  = 0   (no unsigned ninth bit)
Overflow V = 1 (positive + positive became negative)
Negative N = 1 (bit 7 in $80 is set)
Zero Z = 0     ($80 is not zero)
```

In an eight-bit signed view, `$7F` is +127 and `$80` is -128. Thus the CPU can have **signed overflow without unsigned carry**.

Now take subtraction. [`SBC()`](../NESEmulator/cpu6502.cpp) computes `A + (~M & $FF) + C`. Carry set on entry means **no borrow**. For A=`$00`, M=`$01`, C=1, the sum is `$00+$FE+1=$00FF`: result `$FF`, Carry=0 (borrow occurred), Negative=1, Zero=0. In the NES 2A03, decimal-mode arithmetic is not ordinarily used even though the D status bit exists.

The instruction flags live in CPU architectural state. Later branches, interrupts, debugger displays and stack restoration see those same bits; they are not merely explanatory UI labels.

## 5. Page crossing costs time

Memory addresses have a high byte (page) and a low byte. For `ABX()`, let the operand address be `$12FF` and X be 1:

```text
base $12FF + X $01 = effective $1300
before page $12 != after page $13
ABX returns 1, meaning a page was crossed.
```

For an eligible read operation that also returns 1, `1 & 1 = 1` extra cycle. For an operation not eligible, `1 & 0 = 0`. A relative branch uses a signed eight-bit offset instead: `$FE` sign-extends to -2. BEQ adds one cycle when Z=1 (taken), and another if its target crosses a 256-byte page boundary.

The [`IND()`](../NESEmulator/cpu6502.cpp) implementation intentionally reproduces a hardware quirk: `JMP ($02FF)` fetches its target's high byte from `$0200`, not `$0300`. Replacing this with a clean generic two-byte read would reduce compatibility.

## 6. A CPU write to $2005 changes a PPU latch

The CPU-visible PPU registers occupy `$2000–$2007`, repeated through `$3FFF`. `Bus::cpuWrite` applies `addr & 7`. For `$2005` (scroll), the PPU expects two writes:

```text
First write: X scroll = decimal 13
    coarse_x = 13 >> 3 = 1 tile
    fine_x   = 13 & 7  = 5 pixels
    address_latch = 1 (expect Y)

Second write: Y scroll = decimal 19
    coarse_y = 19 >> 3 = 2 tiles
    fine_y   = 19 & 7  = 3 pixels
    address_latch = 0 (expect X)
```

Two writes to `$2006` similarly assemble high and low bytes of a 14-bit PPU VRAM address. A **live** read of PPUSTATUS `$2002` clears the vertical blank flag and resets the address/scroll write latch. A debugger-only read must not cause those side effects; this source has a `rdonly` path.

The CPU's `$2007` data register is backed by PPU-space reads/writes. Ordinary VRAM reads are buffered; palette addresses `$3F00+` bypass the usual one-read delay. The VRAM address advances by 1 or by 32 according to control settings.

## 7. Tiles, pattern bytes and mirroring

The PPU's internal address ranges have distinct meaning:

| PPU address | Data | Owner |
| --- | --- | --- |
| `$0000–$1FFF` | Pattern bits | Cartridge CHR ROM/RAM via active Mapper |
| `$2000–$3EFF` | Nametable tile IDs/attributes | PPU name RAM via mirroring |
| `$3F00–$3FFF` | Palette colors | PPU palette RAM and alias rules |

Vertical nametable mirroring makes `$2000` and `$2800` share storage; horizontal mirroring makes `$2000` and `$2400` share storage. Palette `$3F10` aliases `$3F00`. A correct tile-pattern byte can still display as the wrong pixel if the name-table mirror or attribute palette is wrong.

CHR bank switching from `Mapper_004` changes which physical pattern bytes appear at the same PPU address. The mapper supplies an offset; `Cartridge` validates that offset before accessing its owned CHR memory.

## 8. Two bit planes become one pixel

A background tile has two bit planes. Each contributes one bit for the current raster column; combine `highBit<<1 | lowBit` to obtain a two-bit pixel index 0–3. The PPU stores background pattern and attribute bits in 16-bit shifters and chooses a position with `bit_mux = 0x8000 >> fine_x`.

For `fine_x=3`, the mask is `$1000`. Read the corresponding bit from the low and high pattern shifters to compute the 2-bit pixel. The same bit mask extracts the two attribute bits selecting a subpalette. **Pixel index 0 is transparent** in the layer composition; it is not simply RGB black.

The sprite (foreground) pipeline produces its own pixel index, palette and priority flag. The final selection is:

| Background index | Sprite index | Selected |
| --- | --- | --- |
| 0 | 0 | Backdrop color |
| 0 | Nonzero | Sprite |
| Nonzero | 0 | Background |
| Nonzero | Nonzero | Sprite or background according to sprite priority |

Sprite-zero hit is an additional overlap condition, gated by enabled background/sprite rendering and left-edge clipping policy. It is not synonymous with whichever RGB color is eventually drawn.

The PPU writes the resulting color using `sprScreen->SetPixel(cycle - 1, scanline, GetColourFromPaletteRam(palette,pixel))`. Host PixelGameEngine presents that Sprite; it does not decide NES hardware compositing rules.

## 9. Timing joins CPU, video, DMA and interrupts

[`Bus::clock()`](../NESEmulator/Bus.cpp) advances the PPU each master tick and gives the CPU a clock slot every third tick, unless OAM DMA steals the slot. A CPU write to `$4014` schedules the 256-byte OAM transfer; the bus alternates memory reads and PPU OAM writes instead of letting the CPU execute instructions during the transfer.

At scanline 241, dot 1, the PPU sets vertical blank and can assert an NMI. The Bus forwards it to `cpu6502::nmi()`, which saves PC and status through CPU stack memory and jumps through vector `$FFFA/$FFFB`. Ordinary IRQ uses `$FFFE/$FFFF` and can be masked by the CPU I bit; NMI cannot.

On MMC3/Mapper 004, program `$8000/$A000/$C000/$E000` maps four 8 KiB PRG windows. A simplified `scanline()` counter can assert IRQ. With reload=2 and IRQ enabled, successive calls produce counter 2, then 1, then 0 (IRQ). Real MMC3 IRQ timing depends on qualified PPU A12 transitions, **not** arbitrary “one per scanline” counting. The code is an approximation, not a hardware timing reference.

## 10. Exercises that build genuine understanding

**Trace the CPU.** Place `A9 7F 69 01` at CPU `$8000`, use a reset vector targeting that address, and predict A/C/V/N/Z and instruction countdown after both opcodes. Compare a known-good independent 6502 trace before claiming correctness.

**Trace the scroll.** Write `$0D` then `$13` to CPU `$2005`, inspect coarse/fine scroll fields and latch, then read `$2002` via debugger-only versus live access. Explain why only the live read resets the write phase.

**Trace one pixel.** Choose fine_x=3 and two 16-bit shifters. Compute the `$1000` mask, two-bit pixel index and attribute palette. Overlay one nonzero sprite, flip its priority flag, and predict the output.

**Trace MMC3 IRQ.** Read `Mapper_004::scanline()`, `irqState()`, `Bus::clock()` and `cpu6502::irq()` in that order. Explain the counter and PC/vector updates, then list which timing edges a real NES hardware trace would still need to verify.

**Verification boundary.** Current portable [iNES input regression](../NESEmulator/tests/INES1CartridgeInputTests.cpp) and [controller regression](../NESEmulator/tests/ControllerSerialTests.cpp) do not exercise the instruction/raster examples above or establish full emulator timing correctness. Existing `testing6502.cpp` / `testing2C02.cpp` are demo sources rather than registered CTest conformance tests. A future headless synthetic NROM execution harness and deterministic 256×240 pixel reference tests are the right next step.

The broader engineering connection is the same one seen in Vulkan: *an address is a reference, not a guarantee of valid storage; a CPU action can have device side effects; and an apparently correct frame is not evidence of correct underlying timing*. Teaching the machine means following each byte and state transition through its actual owners.
