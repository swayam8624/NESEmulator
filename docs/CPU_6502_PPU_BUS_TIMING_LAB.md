# NES emulator laboratory — one CPU instruction and one PPU pixel

Read [Bus.cpp](../NESEmulator/Bus.cpp), [cpu6502.cpp](../NESEmulator/cpu6502.cpp), [ppu2C02.cpp](../NESEmulator/ppu2C02.cpp), [Cartridge.cpp](../NESEmulator/Cartridge.cpp), [Mapper_004.cpp](../NESEmulator/Mapper_004.cpp) and the larger [NES architecture guide](NES_ARCHITECTURE_GUIDE.md).

This lesson concerns source-level behavior and approximation limits. Compatibility requires independent CPU instruction traces, PPU raster tests and mapper-specific ROM fixtures. The emulator code's original-contributions MIT notice is not a license to redistribute proprietary NES ROM images.

## 1. The 6502 does not have 64 KiB of uniform RAM

The CPU has a 16-bit address bus, but connected hardware decides what an address means. Bus::cpuRead and cpuWrite delegate cartridge/mapper access first, then map local devices.

- $0000–$1FFF: 2 KiB physical CPU RAM mirrored across an 8 KiB CPU address window. Masking with $07FF maps e.g. $0001, $0801, $1001, $1801 to the same byte.
- $2000–$3FFF: eight CPU-visible PPU registers mirrored. Masking by $0007 maps $2002 and $200A to the same PPU status register.
- $4014: OAM DMA request, copying a 256-byte CPU-memory page into PPU sprite OAM while the CPU is stalled.
- $4016/$4017: controller latch/serial read interactions; reads can advance the shift register.
- Cartridge PRG addresses: forwarded to mapper-selected ROM/RAM banks, not necessarily a fixed vector.

**Exercise:** write RAM byte $5A through $0001 and read from $1801. Explain the identical low 11 address bits. Then compare a live PPU status read and debugger read-only request, and identify which status/latch side effects differ.

**Later source fix:** the controller path now uses [`ReadControllerSerialBit`](../NESEmulator/ControllerSerial.hpp), preserving latched bits when `bReadOnly=true`, and $4016 writes snapshot both players. [Portable controller tests](../NESEmulator/tests/ControllerSerialTests.cpp) cover interleaved debugger peeks. Hardware-accurate strobe edges and post-eight-bit reads remain independent compatibility work.

## 2. From program counter to one logical instruction

cpu6502::clock reads the opcode at the current PC when its cycle countdown reaches zero, increments PC, finds the opcode's addressing function and operation function in a lookup table, then executes both. It computes the base instruction cycles plus any allowed addressing/opcode extra cycle and counts down until the next instruction.

The addressing mode computes an effective address or operand location, while the operation manipulates registers, status flags, stack or memory. That separation explains why LDA immediate, zero-page and absolute all share load logic but use different bus transactions.

This implementation applies the logical instruction's result at the **start** of its nominal cycle window. Real 6502 hardware performs different bus micro-operations on successive cycles, so hardware-sensitive mid-instruction mapper/PPU interactions can differ even if final CPU register traces match.

**Exercise:** write a minimal instruction trace from known reset PC, opcode, addressing mode, initial A/X/Y, status flags and expected remaining cycle count. Compare result after first cpu6502::clock versus after the full instruction duration. A CPU trace match does not certify per-cycle bus fidelity.

## 3. Carry and signed overflow are not the same

For ADC, the unsigned 16-bit intermediate is A + M + CarryIn. The low eight bits become accumulator A; the ninth bit sets carry. The Zero and Negative flags refer to the low byte. Signed overflow V indicates when two operands with the same sign produce a result with the opposite sign.

**Worked example:** A=$7F (127), M=$01, CarryIn=0. Result is $80 (−128 as signed eight-bit), so Carry=0, Negative=1, Overflow=1, Zero=0.

**Another example:** A=$FF, M=$01, CarryIn=0 gives unsigned $100, stored A=$00. Carry=1 and Zero=1, but signed overflow V=0 because the original signed operands have different signs.

For SBC, subtract with *inverted borrow*: A−M−(1−C) is implemented as A+onesComplement(M)+C. Thus carry=1 indicates no borrow, not “negative result.” This is why blindly reusing unsigned subtraction semantics creates incorrect flag behavior.

**Exercise:** build a table for ADC/SBC near $7F,$80,$FF,$00 and compare all four C,V,N,Z flags against a reference 6502 test suite.

## 4. Two important vector and addressing details

Reset loads PC from the reset vector at $FFFC/$FFFD, NMI from $FFFA/$FFFB and maskable IRQ/BRK from $FFFE/$FFFF. The CPU pushes return state to stack page $0100 (using an 8-bit stack pointer). IRQ respects the I flag, whereas NMI does not.

JMP indirect has a 6502-specific quirk: a pointer at $12FF reads the target low byte from $12FF, but its high byte from $1200 rather than $1300. The emulator's IND addressing mode intentionally implements that page-wrap behavior. This is hardware compatibility, not an arbitrary endianness rule.

**Exercise:** place bytes low=$34 at $12FF, high=$56 at $1200, high=$99 at $1300. An indirect JMP through $12FF must target $5634, not $9934.

Relative branches use a signed 8-bit displacement added to PC. An eligible taken branch can cost an additional cycle for taking it and another when crossing a 256-byte page boundary. Inspect the exact branch opcode and addressing-mode return flags rather than inferring all branch cycles from a lookup-table base value.

## 5. Three PPU clocks for a CPU clock, unless DMA steals it

Bus::clock invokes PPU and APU clocks on every master emulation tick and calls cpu6502::clock on every third tick when no OAM DMA transfer is underway.

When DMA is active, the CPU is stalled while the bus alternates reads from a CPU page and writes to PPU OAM, with a dummy alignment phase. Each transfer handles 256 bytes, so the stall spans hundreds of CPU slots. This timing affects sprite setup and which CPU instructions complete before the next PPU vblank.

The bus then calculates whether an audio sample is ready and delivers pending PPU NMI or cartridge mapper IRQ requests to CPU.

**Exercise:** trace the master counter across the start of DMA at $4014, capture 256 source addresses and 256 OAM writes, and count the CPU slots during which cpu6502::clock is not called. Compare parity of the starting counter and any dummy alignment difference.

## 6. PPU registers and buffered reads

The PPU has a separate VRAM address bus from the CPU's $2000–$3FFF *register mapping*. PPU reads/writes resolve cartridge CHR patterns, nametable mirroring and palette memory with a different address interpretation.

A CPU read of PPU status includes vblank/sprite flags and may clear vblank plus the scroll/address write latch. PPU data reads can use a read buffer, so repeatedly reading a VRAM address may not return the value a naive flat-memory model expects.

**Exercise:** write scroll and address bytes in different orders and inspect the Loopy temporary/current VRAM address and fine X latch. Verify why a status read resets the alternating write phase.

## 7. How one visible pixel appears

ppu2C02::clock advances the raster's cycle/scanline state machine. Background fetch logic reads nametable tile IDs, attribute palettes and low/high pattern bytes, storing them in shifters that contribute a 2-bit background pixel. Fine X scroll selects the correct bit within each tile's eight-pixel span.

Sprite evaluation selects a limited set of active scanline objects, with per-sprite X counters, pattern shifters, palette flags and priority bits. The foreground pixel is the first nontransparent candidate in sprite order; background/foreground transparency and priority decide final output.

Sprite-zero hit is a particular raster event when nontransparent background and sprite-zero pixels overlap under the appropriate rendering conditions. It is not a physics-world collision or an intersection test of two AABBs.

**Exercise:** create a test scanline with nontransparent background at x=16 and a nontransparent sprite-zero pixel at x=16. Follow pixel priority and sprite-zero hit state. Repeat with transparent background or sprite and explain why the event differs.

## 8. Mapper timing and compatibility limits

Cartridge owns PRG/CHR data and a Mapper_* strategy. MMC3-style Mapper_004 bank-switches program and character memory and requests IRQ as scanlines advance. Precise hardware MMC3 IRQ behavior depends on qualified PPU A12 transitions, so a simplified scanline notification cannot be assumed cycle-accurate.

**Later source fix:** [the bounded iNES1 reader](../NESEmulator/INES1Header.hpp) checks signature, supported mapper, complete optional trainer/PRG/CHR payload and CHR-RAM semantics, while [Cartridge.cpp](../NESEmulator/Cartridge.cpp) bounds mapper-produced offsets. NES 2.0 and four-screen formats remain unsupported; a passing CPU input test does not certify all cartridge gameplay or malicious-input resilience. Read [the deep boundary lab](NES_CARTRIDGE_TRUST_BOUNDARY_AND_CONTROLLER_IO_LAB.md).

## 9. Test-driven learning plan

For CPU: deterministic register/instruction traces, ADC/SBC exhaustive flag tables, indirect JMP wrap, branch page crossings, stack/interrupt vectors and DMA stall parity. For PPU: status side effects, scroll latch, nametable mirroring, palette data buffer, dot timing, sprite zero, vblank/NMI, CHR bank mapping and scanline IRQ. For integration: verify the same legal test ROM and independent trace at pinned source SHA.

The host's SDL/OpenGL pixel presentation or emulator audio output is a **consumer** of those simulated states, not an oracle of hardware fidelity. For visual screenshots, preserve legal provenance for ROMs and the underlying test assets.

*Evidence status: source-derived teaching material. No new emulator run, NES hardware trace or compatibility benchmark was executed.*
