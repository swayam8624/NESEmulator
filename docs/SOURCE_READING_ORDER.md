# Teaching source map — NESEmulator

> A guided source-reading itinerary. It is not a test result or claim that every file is documented yet.

## How to use this guide

**Source-review warnings:** Cartridge.cpp currently lacks complete iNES
header/payload/unsupported-mapper validation, and Bus::cpuRead shifts
controller state even when bReadOnly is requested for a controller
address. These are documented limitations, not fixes in this branch.
See [the hardware walkthrough](NES_ARCHITECTURE_GUIDE.md) and tests/manual
trace tools before claiming untrusted-ROM safety or debugger purity.

Follow one frame from user input to the CPU/PPU bus. The app uses a C++23 module interface; its implementation units share `module Kairo.NES;`.

Read each phase in order. Every link goes to a source file or directory in this repository. For a directory, inspect its public interface and then its implementation files. After each phase, identify the caller, the state it owns, the inputs it validates, and its observable outputs.

## 1. Boot and public API

1. [`README.md`](../README.md)
2. [`CMakeLists.txt`](../CMakeLists.txt)
3. [`NESEmulator/KairoNES.cppm`](../NESEmulator/KairoNES.cppm)
4. [`NESEmulator/main.cpp`](../NESEmulator/main.cpp)

## 2. Cartridge and memory

1. [`NESEmulator/Cartridge.cpp`](../NESEmulator/Cartridge.cpp)
2. [`NESEmulator/Mapper.cpp`](../NESEmulator/Mapper.cpp)
3. [`NESEmulator/Mapper_000.cpp`](../NESEmulator/Mapper_000.cpp)
4. [`NESEmulator/Mapper_001.cpp`](../NESEmulator/Mapper_001.cpp)
5. [`NESEmulator/Mapper_002.cpp`](../NESEmulator/Mapper_002.cpp)
6. [`NESEmulator/Mapper_003.cpp`](../NESEmulator/Mapper_003.cpp)
7. [`NESEmulator/Mapper_004.cpp`](../NESEmulator/Mapper_004.cpp)
8. [`NESEmulator/Mapper_066.cpp`](../NESEmulator/Mapper_066.cpp)

## 3. Execution hardware

1. [`NESEmulator/Bus.cpp`](../NESEmulator/Bus.cpp)
2. [`NESEmulator/cpu6502.cpp`](../NESEmulator/cpu6502.cpp)
3. [`NESEmulator/ppu2C02.cpp`](../NESEmulator/ppu2C02.cpp)
4. [`NESEmulator/apu2A03.cpp`](../NESEmulator/apu2A03.cpp)

## 4. Host rendering and diagnostics

1. [`NESEmulator/KairoPixelGameEngine.cppm`](../NESEmulator/KairoPixelGameEngine.cppm)
2. [`NESEmulator/testing6502.cpp`](../NESEmulator/testing6502.cpp)
3. [`NESEmulator/testing2C02.cpp`](../NESEmulator/testing2C02.cpp)

## End-to-end interaction map

`main.cpp` converts keyboard input to controller bits; `Bus.cpp` maps CPU addresses to RAM, cartridge, PPU, APU and DMA. `Cartridge.cpp` asks `Mapper_*` to translate ROM bank addresses. CPU instructions and PPU pixels advance under `Bus::clock()`. The host pixel engine displays the frame and handles input/audio. External ROM data is not source code; never distribute commercial ROMs without permission.

## APIs and code-reading questions

For each function, distinguish **contract** (preconditions, postconditions, failure modes), **implementation** (state/algorithms), **collaborators** (imports, calls and consumers), **lifetime/ownership**, and **verification** (test or evidence that checks it). For C++ modules, `export module` names the public compiled interface and `import` expresses module dependencies; an implementation unit's `module ...;` contributes definitions without exporting them.

## Boundaries and honesty

The current tree includes prebuilt executables and no CTest/Actions coverage. Treat APU and mapper compatibility as work to validate, not established console-wide accuracy. Review the provenance of the adapted pixel engine and emulator components before redistribution.

## Documentation rules

- Teaching comments describe *why*, invariants, edge cases, API ownership and cross-file effects; avoid narrating obvious assignments.
- Preserve behavior during documentation-only edits; keep correctness changes in separate reviewed commits.
- Link function names and tests where possible; mark claims as source-present, CI-observed, host-verified or research-evidenced.
- License grants apply only to material for which the repository owner holds rights; third-party source, models, media and datasets keep their own notices.


## Complete source-file inventory

This ordered inventory supplements the **curated** reading path above. It includes every first-party code, build, script, shader and test file found in the current teaching branch at the time of indexing. Read the curated path **first**, then work down the categories below, following imports and calls when a dependent file precedes its implementation. External/vendor source, binaries, test ROMs and generated evidence are deliberately excluded from this source-learning checklist; their rights and verification are independent.

### Entry points and build (2)

1. [`CMakeLists.txt`](../CMakeLists.txt)
2. [`NESEmulator/main.cpp`](../NESEmulator/main.cpp)

### Interfaces and modules (2)

1. [`NESEmulator/KairoNES.cppm`](../NESEmulator/KairoNES.cppm)
2. [`NESEmulator/KairoPixelGameEngine.cppm`](../NESEmulator/KairoPixelGameEngine.cppm)

### Implementation and other source (14)

1. [`NESEmulator/apu2A03.cpp`](../NESEmulator/apu2A03.cpp)
2. [`NESEmulator/Bus.cpp`](../NESEmulator/Bus.cpp)
3. [`NESEmulator/Cartridge.cpp`](../NESEmulator/Cartridge.cpp)
4. [`NESEmulator/cpu6502.cpp`](../NESEmulator/cpu6502.cpp)
5. [`NESEmulator/Mapper_000.cpp`](../NESEmulator/Mapper_000.cpp)
6. [`NESEmulator/Mapper_001.cpp`](../NESEmulator/Mapper_001.cpp)
7. [`NESEmulator/Mapper_002.cpp`](../NESEmulator/Mapper_002.cpp)
8. [`NESEmulator/Mapper_003.cpp`](../NESEmulator/Mapper_003.cpp)
9. [`NESEmulator/Mapper_004.cpp`](../NESEmulator/Mapper_004.cpp)
10. [`NESEmulator/Mapper_066.cpp`](../NESEmulator/Mapper_066.cpp)
11. [`NESEmulator/Mapper.cpp`](../NESEmulator/Mapper.cpp)
12. [`NESEmulator/ppu2C02.cpp`](../NESEmulator/ppu2C02.cpp)
13. [`NESEmulator/testing2C02.cpp`](../NESEmulator/testing2C02.cpp)
14. [`NESEmulator/testing6502.cpp`](../NESEmulator/testing6502.cpp)

Source-index snapshot: 18 code/build/test files, 21 other tracked files (including excluded third-party, documentation and assets). Use a new tree audit before treating the index as current after subsequent source changes.
