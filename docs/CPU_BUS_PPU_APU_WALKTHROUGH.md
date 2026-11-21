# From 6502 instruction to rendered pixel: a guided emulator trace

This is a detailed companion to [SOURCE_READING_ORDER.md](SOURCE_READING_ORDER.md). It explains the **implemented C++ control flow** first, and distinguishes that from the behavior of a fully hardware-accurate NES. All links are to this repository's source. Some of this code appears influenced by external emulator tutorials; read [THIRD_PARTY_AND_ROMS.md](THIRD_PARTY_AND_ROMS.md) before redistributing source or ROMs.

## 1. What the machine components represent

The `Bus` owns the emulator's composition and high-level clock. The `cpu6502` executes instructions and asks the bus for bytes. `Cartridge` owns loaded PRG/CHR storage and delegates bank switching to a `Mapper` subclass. The `ppu2C02` implements the CPU-visible PPU registers **and** the PPU's own VRAM/pixel pipeline. The `apu2A03` accepts audio registers and produces a mixed sample.

The [C++23 module](../NESEmulator/KairoNES.cppm) gathers their declarations. The [host application](../NESEmulator/main.cpp) initializes the emulator, loads a cartridge and repeatedly drives its clock while presenting image/audio/control state. The [KairoPixelGameEngine module](../NESEmulator/KairoPixelGameEngine.cppm) supplies host/window/pixel abstractions; its provenance must be reviewed independently.

These are different kinds of time: **a host display frame**, **a PPU master-clock tick**, **a CPU clock tick**, **a complete instruction**, and **a sound output sample**. Treating them as interchangeable produces timing errors.

## 2. Start with one CPU memory read

The CPU has a 16-bit address space. Every instruction requiring a bus byte calls [`cpu6502::read`](../NESEmulator/cpu6502.cpp), which delegates to [`Bus::cpuRead`](../NESEmulator/Bus.cpp). That function routes requests in priority order:

| CPU address | Implemented route | What the reader should notice |
|---|---|---|
| `$0000-$1FFF` | `cpuRam[addr & 0x07FF]` | Only 2 KiB of RAM; the larger range mirrors it four times |
| `$2000-$3FFF` | `ppu.cpuRead(addr & 7)` | Eight CPU-visible PPU registers repeated across the range |
| `$4000-$4013`, `$4015`, `$4017` writes | APU | Sound generator/register control |
| `$4014` writes | OAM DMA setup | Queue 256 bytes of sprite OAM transfer, temporarily taking CPU bus cycles |
| `$4016-$4017` reads | Controller shift state | Each read shifts a latched serial bit; a debugger read may be destructive |
| Cartridge-mapped regions | `cart->cpuRead/Write` | Mapper decides PRG banking, PRG RAM and register ownership *before* normal bus routing |

The order is an important **control-flow invariant**. `Cartridge::cpuRead` asks `Mapper::cpuMapRead` whether it owns an address, then either reads the mapper-supplied byte or indexes PRG memory using the returned translated offset. A sentinel value `0xFFFFFFFF` means the mapper supplied the data itself (for example from cartridge RAM). The same address may point to different PRG data after a bank-register write.

Follow these files together: [Bus.cpp](../NESEmulator/Bus.cpp), [Cartridge.cpp](../NESEmulator/Cartridge.cpp), [Mapper_000.cpp](../NESEmulator/Mapper_000.cpp) and [Mapper_004.cpp](../NESEmulator/Mapper_004.cpp). Mapper 0 is a useful first bank-mapping example; Mapper 4 illustrates switched PRG/CHR banks and IRQ state.

### Experiment: reset-vector lookup

[`cpu6502::reset`](../NESEmulator/cpu6502.cpp) performs two bus reads at `$FFFC` and `$FFFD`, combines them into the program counter, and initializes stack/status registers. Draw the call chain:

```text
cpu6502::reset
  -> cpu6502::read($FFFC/$FFFD)
    -> Bus::cpuRead
      -> Cartridge::cpuRead
        -> Mapper_XXX::cpuMapRead
          -> vPRGMemory[translated offset] or mapper-supplied byte
```

The reset vector is part of the cartridge's **CPU-visible address map**, not a magical hardcoded C++ function address. Without the correct mapper, even an otherwise valid ROM cannot start at the intended instruction.

## 3. Opcode dispatch, addressing modes and flags

[`cpu6502::clock`](../NESEmulator/cpu6502.cpp) executes the entire logical instruction when its remaining `cycles` counter is zero. It reads an opcode, selects a lookup-table entry, calls the addressing-mode function (e.g., `IMM`, `ABS`, `ABX`, `IZY`), then the operation function (e.g., `LDA`, `ADC`, `STA`). The two callbacks may request an additional cycle, which is reflected in the countdown.

Addressing modes answer **where is the operand?** Operation handlers answer **what do we do with it?** For example, `ADC` must combine the fetched byte with the accumulator and carry bit, then update status flags. Do not confuse unsigned carry with signed overflow.

Most later `cpu6502::clock` calls simply decrement the remaining cycle count. Instruction effects are not modeled as individual bus micro-operations at every tick. This is practical for a teaching emulator but has **important accuracy limitations** for bus-side-effect timing and exact hardware tests.

Try tracing the following three examples with your own legal test program or assembler-generated ROM:

- `LDA #$10`: immediate addressing reads a literal operand into A, then updates zero/negative status.
- `STA $2000`: absolute addressing reaches a CPU-visible PPU control register through `Bus::cpuWrite`.
- `JMP ($xxxx)`: indirect addressing uses a pointer read through the bus; hardware compatibility may require observing the 6502 page-wrap behavior.

For interrupts, compare `irq` (maskable), `nmi` (non-maskable) and `BRK` (software). They save processor state on the stack and use different vector semantics. In this project, [Bus.cpp](../NESEmulator/Bus.cpp) forwards the PPU's NMI request and the active mapper's IRQ request.

## 4. A PPU register write is not a VRAM write

From the CPU's perspective, writing `$2006` or `$2007` is writing a **register**. From the PPU's perspective, these accesses control or perform operations in its own 14-bit graphics address space. This boundary sits in [ppu2C02.cpp](../NESEmulator/ppu2C02.cpp):

1. CPU `STA` or similar store → `Bus::cpuWrite` → `ppu.cpuWrite(addr & 7)`.
2. PPU control/scroll/address registers update internal state, including the address latch and scrolling fields.
3. The PPU's tile-fetch pipeline later calls `ppuRead`, first offering CHR pattern-table accesses to the cartridge mapper, then handling name-table and palette storage itself.
4. As pixels are generated, sprite and background values are composed using palette, priority and sprite-zero state.

PPU address regions are also distinct from CPU bus regions: `$0000-$1FFF` contains cartridge CHR/pattern-table access, `$2000-$3EFF` maps internal name tables through horizontal/vertical mirroring, and `$3F00-$3FFF` is palette storage.

[`ppu2C02::cpuRead`](../NESEmulator/ppu2C02.cpp) has meaningful side effects. A normal read of `PPUSTATUS` clears its vertical-blank flag and scroll/address latch. `PPUDATA` uses a delayed read buffer for normal VRAM data but handles palette reads immediately; the VRAM address then increments according to the control register. The `rdonly` path is intended for debugging without those state changes.

### Experiment: name-table mirroring

Sketch the four logical name-table quadrants and map them to physical `tblName` arrays for both horizontal and vertical mirroring. Find where [`Cartridge::Mirror`](../NESEmulator/Cartridge.cpp) chooses fixed iNES flags versus dynamic mapper overrides. Changing bank selection and changing mirror policy are **separate** operations.

## 5. One system clock schedules four concerns

[`Bus::clock`](../NESEmulator/Bus.cpp) is the scheduling heart of this implementation:

- Advance PPU and APU for each emulated master tick.
- Every third master tick, advance the CPU **unless OAM DMA has taken bus ownership**.
- During DMA, alternate reading a CPU bus byte and writing the PPU's `pOAM` array until 256 bytes are transferred.
- Update the host-audio resampling accumulator. When a sample is ready, ask `apu.GetOutputSample()` for the current mix.
- Check and forward PPU NMI and mapper IRQ requests.

This means a CPU register write to `$4014` is not an instantaneous sprite-memory copy: it **schedules** future bus use. Likewise a write to APU pulse registers changes channel configuration but the host sample emerges only when the audio accumulator requests it.

[`Mapper_004::scanline`](../NESEmulator/Mapper_004.cpp) updates an IRQ counter that can eventually trigger `cpu6502::irq()`. Treat this as the source implementation's scanline-oriented approximation rather than automatically assuming hardware-exact PPU A12-edge IRQ timing.

### Experiment: audio output

Trace a pulse-channel timer write from `cpuWrite` through [apu2A03.cpp](../NESEmulator/apu2A03.cpp), then `clock()` and `GetOutputSample()`. Explain why sample readiness and oscillator state advancement are different operations. Note that the current APU-status read implementation has commented-out status bits.

## 6. Known limitations to be aware of while learning

These are observations about the **current source**, not assertions about how NES hardware should behave:

1. **ROM parser/mapper validation:** `Cartridge::ImageValid` can report success after an opened file even if the mapper-switch statement never assigned `pMapper`. Read paths dereference that pointer without a comprehensive malformed-file/unsupported-mapper validation layer. This needs hardening before treating arbitrary ROM files as safe.
2. **Cycle fidelity:** 6502 execution commits instruction effects at decode and counts down cycles. This simplifies teaching but is not identical to a bus-cycle-accurate core.
3. **APU completeness:** `apu2A03::cpuRead($4015)` leaves channel-status logic commented out, and `apu2A03::reset()` has an empty body. The bus reset also does not call APU reset.
4. **Controller/APU address sharing:** in `Bus::cpuWrite`, `$4017` is matched by the APU write branch before the later controller-latch branch. The ordering is worth investigating with a dedicated second-controller test instead of assuming both branches execute.
5. **I/O inspection:** `Bus::cpuRead` shifts controller state even when its caller requested `bReadOnly`; the PPU itself has a separate debugger-read path. Check the exact device before using debugger reads as passive observations.
6. **Copyright and redistribution:** existing ROMs and externally derived code are not automatically licensed by new educational documentation. This project intentionally does **not** apply a blanket MIT license to third-party source or ROMs.

These caveats make useful **test-driven exercises**. Begin with reproducible unit tests or clean-room ROM fixtures; do not quietly change emulator behavior while adding explanatory comments.

## 7. Practice and evidence

Read [testing6502.cpp](../NESEmulator/testing6502.cpp) for CPU-facing test/debug facilities, then [testing2C02.cpp](../NESEmulator/testing2C02.cpp) for PPU-facing inspection. For each bug hypothesis, document the input ROM or synthetic test bytes, expected state after a measured number of clocks, actual state, and an exact replay procedure.

To study one instruction well, your notes should answer: which address was generated, which device owned it, whether the access had side effects, how many emulator ticks advanced, which flags changed, and how the next pixel/audio state could depend on the outcome. That is the conceptual path from an 8-bit opcode to a visible/emitted result.
