# KairoNES: source reading order, ownership, and API relationships

> Read this guide alongside the code, not instead of the code. Each link opens an actual file. The priority is **understanding why each component exists**, how control/data crosses files, and what the public APIs do. A source file's existence does not prove full NES compatibility. Begin with [README.md](README.md) for the macOS toolchain.

## Mental model: three flows that must not be confused

```text
Host application (main.cpp, PixelGameEngine)
  | keys                     | host frame/update               | draws/audio
  v                          v                                 ^
Bus.controller[]        Bus::clock() ------------------------> PPU screen / audio sample
  |                          |            |           |
  | CPU reads $4016          |            |           +--> apu2A03::clock()
  +--------------------------+----> cpu6502::clock() --------> Bus::cpuRead / cpuWrite
                             +----> ppu2C02::clock() -----+       |
                                                         |       v
                                                         +--> Cartridge
                                                               |
                                                               v
                                                        Mapper_000..066
```

**Control flow:** a PixelGameEngine callback advances emulation via the Bus. **Address flow:** the CPU uses *CPU addresses*, while the PPU separately uses *PPU addresses*. **Ownership:** the Bus connects hardware, the Cartridge holds PRG/CHR bytes, and a Mapper computes bank offsets but does not own ROM data. Keep these three questions separate when debugging.

## Reading path — each source file, in order

### Part 1 — Contracts and the simplest cartridge

1. [`NESEmulator/KairoNES.cppm`](NESEmulator/KairoNES.cppm): Begin with `export module Kairo.NES;`. This is a **C++23 module interface** defining the types imported by `main.cpp`. Identify `cpu6502`, `ppu2C02`, `apu2A03`, `Mapper`, `Cartridge`, and `Bus`. Study their public methods, data ownership, and IRQ/NMI signals before viewing implementations. `module Kairo.NES;` in the other source files means they supply the exported module's implementation, not a second module.
2. [`NESEmulator/Mapper.cpp`](NESEmulator/Mapper.cpp): Learn the abstract address-translation interface. `cpuMapRead`, `cpuMapWrite`, `ppuMapRead`, and `ppuMapWrite` translate an input address into a cartridge-memory offset or mapper-owned result. `mirror`, `scanline` and `irqState` are extension points.
3. [`NESEmulator/Mapper_000.cpp`](NESEmulator/Mapper_000.cpp): NROM has fixed bank layout. Sketch why a 16-KiB PRG ROM may mirror into a 32-KiB CPU window, and why absent CHR ROM can imply CHR RAM.
4. [`NESEmulator/Cartridge.cpp`](NESEmulator/Cartridge.cpp): Follow `Cartridge::Cartridge` from file header to bank allocation and mapper factory. Then follow `cpuRead` and `ppuRead` from the mapper's offset into PRG/CHR storage. **Caveat:** the current loader insufficiently validates truncated/unsupported files; `ImageValid` should not be used as an untrusted-input security guarantee.

### Part 2 — Wiring and CPU execution

5. [`NESEmulator/Bus.cpp`](NESEmulator/Bus.cpp): Trace `cpuRead` and `cpuWrite` through cartridge, internal RAM, PPU registers, APU registers, DMA and controller bits. `Bus::clock` is the timebase: observe PPU/CPU cadence, DMA stalls, audio sampling and NMI/IRQ delivery. Ask how a CPU write to a mapper register can affect a later PPU CHR fetch.
6. [`NESEmulator/cpu6502.cpp`](NESEmulator/cpu6502.cpp): The 256-entry opcode lookup combines operations, addressing modes and cycle counts. First read `clock`, `fetch`, `reset`, `irq`, and `nmi`. Then compare immediate, zero-page, absolute, indexed and indirect modes, followed by `LDA`, `STA`, `ADC`, `BRK`. CPU memory goes through `Bus`; the emulator must not read ROM arrays directly.
7. [`NESEmulator/testing6502.cpp`](NESEmulator/testing6502.cpp): An optional PixelGameEngine CPU visualizer for studying registers, disassembly and memory. It is **not** part of the active CMake executable and is not an automated conformance suite.

### Part 3 — Video and audio

8. [`NESEmulator/ppu2C02.cpp`](NESEmulator/ppu2C02.cpp): Start with the crucial distinction between `cpuRead/cpuWrite` (**CPU-visible PPU registers**) and `ppuRead/ppuWrite` (**PPU's own address space**). Trace nametables, pattern/CHR fetches, palette, scrolling, sprite evaluation, scanline timing and NMI. The eventual screen image is consumed by the frontend.
9. [`NESEmulator/testing2C02.cpp`](NESEmulator/testing2C02.cpp): Optional visual exploration of PPU/pattern/palette state. Do not confuse a screenshot with correctness testing of frame timing, sprite-zero behavior or clipping.
10. [`NESEmulator/apu2A03.cpp`](NESEmulator/apu2A03.cpp): Follow the APU CPU register writes from `Bus`; `clock` updates internal audio channels; `GetOutputSample` supplies the mixed sample used by the Bus's sampling path. The implementation is partial and must not be represented as complete/cycle-accurate sound synthesis.

### Part 4 — Mappers with state and interrupts

11. [`NESEmulator/Mapper_002.cpp`](NESEmulator/Mapper_002.cpp): UxROM uses a selectable PRG bank plus a fixed bank. CPU writes change future reads, not the ROM data.
12. [`NESEmulator/Mapper_003.cpp`](NESEmulator/Mapper_003.cpp): CNROM changes the CHR bank observed by PPU fetches; inspect the distinction between CPU bank control and PPU read consumption.
13. [`NESEmulator/Mapper_066.cpp`](NESEmulator/Mapper_066.cpp): GxROM programs larger PRG/CHR bank selections. Draw the path `CPU write → mapper state → later CPU/PPU read`.
14. [`NESEmulator/Mapper_001.cpp`](NESEmulator/Mapper_001.cpp): MMC1 serially shifts control bits to configure memory banking and mirroring. Compare reset state and how successive writes build configuration.
15. [`NESEmulator/Mapper_004.cpp`](NESEmulator/Mapper_004.cpp): MMC3 adds more bank controls and scanline-associated IRQs. Trace `scanline → irqState → Bus::clock → cpu6502::irq`. Examine what happens when the IRQ counter is enabled/disabled.

### Part 5 — The app shell, vendor API, and build graph

16. [`NESEmulator/main.cpp`](NESEmulator/main.cpp): `Demo_KairoNES` derives from `kairo::PixelGameEngine`. The host calls `OnUserCreate` at startup and `OnUserUpdate` every *host frame*. `PollController` converts `GetKey` results into NES controller bits, and sprite drawing shows PPU pixels. Trace audio callback behavior and remember that host presentation clocks are not the NES chip clocks.
17. [`NESEmulator/KairoPixelGameEngine.cppm`](NESEmulator/KairoPixelGameEngine.cppm): This large host window/pixel/input/audio implementation appears adapted from OneLoneCoder's olcPixelGameEngine. Skim the public `PixelGameEngine` lifecycle, `Pixel`, `Sprite`, `GetKey` and drawing calls. It is not original NES CPU/PPU simulation. Preserve upstream notices: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
18. [`CMakeLists.txt`](CMakeLists.txt): Connect the file set to the `game` target, including `CXX_MODULES`, C++23 compilation, host GLUT/OpenGL/Cocoa/libpng, and copy of ROM fixtures. The two visualizers are not active build targets.

## Core API glossary — who talks to whom?

| API | Caller / consumer | Responsibility and caution |
|---|---|---|
| `import Kairo.NES;` | `main.cpp` | C++ module import; module interface owns declarations, implementation units define them |
| `Bus::clock` | Frontend | Hardware tick scheduling, not a complete host frame |
| `Bus::cpuRead / cpuWrite` | CPU/debugger | CPU address decoding and device side effects |
| `Cartridge::cpuRead / ppuRead` | Bus/PPU | PRG/CHR memory access using mapper bank translations |
| `Mapper::cpuMapRead / ppuMapRead` | Cartridge | Virtual address→offset mapping for the current board |
| `cpu6502::clock` | Bus | One CPU clock; opcode fetch/execute spans multiple clocks |
| `ppu2C02::clock` | Bus | One PPU dot with scanline/frame/render side effects |
| `ppu2C02::cpuRead` | Bus | CPU-side PPU register; separate from `ppu2C02::ppuRead` |
| `apu2A03::GetOutputSample` | Bus | Channel mix consumed at host sample timing; incomplete fidelity |
| `Mapper_004::scanline` | PPU timing path | MMC3 interrupt counter progression |
| `PixelGameEngine::OnUserUpdate` | Host API | Window update callback; must drive emulation independently |
| `PixelGameEngine::GetKey/Draw` | Frontend | Host input/pixel API (third-party-derived), not NES registers |

## Exercises that force architectural understanding

**1. Trace a byte.** Follow `LDA $8000` through CPU fetch → Bus CPU read → Cartridge → mapper → PRG byte. Change the mapper and explain why the same CPU address can return different bytes.

**2. Trace a tile.** Follow a PPU pattern-table fetch → PPU address-space read → Cartridge → CHR bank translation. Compare with the CPU's access path and identify where mirroring applies.

**3. Trace an interrupt.** Explain why an MMC3 IRQ depends on PPU timing even though its target is the CPU. Distinguish it from the PPU's NMI.

**4. Verify rather than trust.** Add a deterministic test runner for CPU opcode flags and cycle counts, PPU timing/sprites, CHR/PRG mapper boundaries, and APU waveforms. Add malformed-ROM and unsupported-mapper negative tests; the repository does not yet contain a complete automatic conformance suite.

## Reading the other project files

[`README.md`](README.md) explains setup and limitations; [`LICENSE`](LICENSE) covers independently owned contributions; [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) explains upstream OLC-3 and ROM restrictions; [`CONTRIBUTING.md`](CONTRIBUTING.md) gives a code-review and testing standard. Generated binaries, IDE metadata, and game ROMs are not source material to read as code. Original creator credits must survive all derivative releases.
