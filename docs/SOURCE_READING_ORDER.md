# Source reading guide — KairoNES

This is a **teaching path**, not a build order. Read the links in sequence, then trace a CPU read, a PPU register write, and a whole emulated frame through the source. The CMake target compiles all the files together; C++ module declarations live in `KairoNES.cppm`, and the corresponding `.cpp` files provide implementations.

## 1. Entry point and system architecture

1. [README](../README.md) — build constraints, controls, and project limitations.
2. [CMakeLists.txt](../CMakeLists.txt) — discover the compiler, source set, module interface, and link libraries.
3. [KairoNES.cppm](../NESEmulator/KairoNES.cppm) — exported declarations and ownership relationships. **Start with `Bus`, `Cartridge`, `Mapper`, `cpu6502`, `ppu2C02`, and `apu2A03`.**
4. [main.cpp](../NESEmulator/main.cpp) — application lifecycle, keyboard input, timing, debugger panels and drawing. Trace `Demo_KairoNES` into `Bus::clock()`.
5. [Bus.cpp](../NESEmulator/Bus.cpp) — the central integration point: address decoding, RAM mirroring, controller latches, DMA, PPU/APU ticking, and CPU clock division.

## 2. Cartridge and memory mapping

6. [Cartridge.cpp](../NESEmulator/Cartridge.cpp) — read iNES metadata, allocate PRG/CHR storage, select a mapper, and translate memory accesses.
7. [Mapper.cpp](../NESEmulator/Mapper.cpp) — common mapper interface and default hooks.
8. [Mapper_000.cpp](../NESEmulator/Mapper_000.cpp) — simplest fixed-bank NROM case.
9. [Mapper_002.cpp](../NESEmulator/Mapper_002.cpp) — CPU bank switching.
10. [Mapper_003.cpp](../NESEmulator/Mapper_003.cpp) — CHR bank switching.
11. [Mapper_066.cpp](../NESEmulator/Mapper_066.cpp) — combined PRG/CHR bank selection.
12. [Mapper_001.cpp](../NESEmulator/Mapper_001.cpp) — serial-register MMC1 control.
13. [Mapper_004.cpp](../NESEmulator/Mapper_004.cpp) — MMC3 bank selection and scanline IRQ behaviour.

**Trace:** `Bus::cpuRead` → `Cartridge::cpuRead` → `Mapper::cpuMapRead` → PRG bank byte. The PPU performs the analogous CHR path. A mapper returns a mapped address, not necessarily the byte itself.

## 3. Device emulation and front end

14. [cpu6502.cpp](../NESEmulator/cpu6502.cpp) — opcode lookup, addressing modes, clock cycles, interrupts and flags. Trace `cpu6502::read` back to the bus.
15. [ppu2C02.cpp](../NESEmulator/ppu2C02.cpp) — PPU register interface, tile/palette fetching, sprites, scrolling and frame production. Trace `ppu2C02::cpuWrite` from the bus.
16. [apu2A03.cpp](../NESEmulator/apu2A03.cpp) — current audio implementation and its limitations.
17. [KairoPixelGameEngine.cppm](../NESEmulator/KairoPixelGameEngine.cppm) — bundled window/input/pixel layer; read selectively, after understanding the NES core.
18. [testing6502.cpp](../NESEmulator/testing6502.cpp) and [testing2C02.cpp](../NESEmulator/testing2C02.cpp) — alternate experimental drivers; **not listed in the active `game` target**.

## Cross-file mental models

**Application input:** keyboard in `main.cpp` → `Bus::controller` → latched serial bits through `Bus::cpuRead(0x4016/0x4017)` → game instruction.

**CPU memory:** CPU instruction in `cpu6502.cpp` → `Bus::cpuRead/cpuWrite` → RAM, PPU/APU register, controller, DMA or `Cartridge` → mapper → PRG memory.

**Video:** application calls `Bus::clock()` → `ppu2C02::clock()` produces the frame → `ppu2C02::GetScreen()` → pixel engine drawing in `main.cpp`.

**DMA:** write to CPU address `0x4014` → bus DMA state → bus copies 256 bytes toward PPU OAM while CPU execution is stalled.

**Audio:** bus clocks APU, but audio is not the application's frame driver; check [README](../README.md) before assuming full hardware-accurate sound.

## Study exercises

1. Put a breakpoint in `Bus::cpuRead()` when an instruction fetch accesses a ROM address; record how the mapper changes that address.
2. Inspect the reset vector path: cartridge inserted → reset → CPU PC initialization.
3. Trace controller button A from physical key to its corresponding bit in a CPU read.
4. Explain why CPU RAM is masked by `0x07FF` and PPU register addresses by `0x0007`.
5. Identify where an invalid/unsupported mapper, malformed ROM, or null cartridge could cause failure. Document findings before changing runtime behaviour.

## Scope and provenance

These links cover **tracked source files**, not IDE metadata, binary ROMs, game executables, or generated assets. Read [THIRD_PARTY_AND_ROMS.md](THIRD_PARTY_AND_ROMS.md) before redistributing any bundled materials. Comments describe the code as implemented; they are not a claim of full NES compatibility.
