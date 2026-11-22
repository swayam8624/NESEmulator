# Teaching source map — NESEmulator

> A guided source-reading itinerary. It is not a test result or claim that every file is documented yet.

**Work a hardware trace:** [CPU address mirroring, ADC/SBC flags, OAM DMA stalls and one PPU raster pixel](CPU_6502_PPU_BUS_TIMING_LAB.md). First read the new [iNES cartridge trust boundary and two-player controller I/O laboratory](NES_CARTRIDGE_TRUST_BOUNDARY_AND_CONTROLLER_IO_LAB.md) to understand which untrusted bytes and state reach the CPU/PPU hardware map. Both are code-based lessons, not claims of cycle-exact equivalence.

## How to use this guide

**Correctness updates:** [the cartridge/input boundary laboratory](NES_CARTRIDGE_TRUST_BOUNDARY_AND_CONTROLLER_IO_LAB.md) now explains the source-level iNES 1.0 header/payload validation, unsupported mapper/NES 2.0 rejection, PRG/CHR bounds, two-player $4016 latch and read-only debugger controller peeks. Portable C++23 tests and a GCC/Clang CI workflow are committed. These targeted guards do not establish general untrusted-ROM or cycle-accurate hardware compatibility.

Follow one frame from user input to the CPU/PPU bus. The app uses a C++23 module interface; its implementation units share `module Kairo.NES;`.

Read each phase in order. Every link goes to a source file or directory in this repository. For a directory, inspect its public interface and then its implementation files. After each phase, identify the caller, the state it owns, the inputs it validates, and its observable outputs.

## 1. Boot and public API

1. [`README.md`](../README.md)
2. [`CMakeLists.txt`](../CMakeLists.txt)
3. [`NESEmulator/KairoNES.cppm`](../NESEmulator/KairoNES.cppm)
4. [`NESEmulator/main.cpp`](../NESEmulator/main.cpp)

## 2. Cartridge and memory

Read the [portable checked iNES1 header/payload parser](../NESEmulator/INES1Header.hpp) and its [synthetic, freely distributable ROM-byte tests](../NESEmulator/tests/INES1CartridgeInputTests.cpp) **before** tracing native Cartridge construction.

1. [`NESEmulator/INES1Header.hpp`](../NESEmulator/INES1Header.hpp)
2. [`NESEmulator/Cartridge.cpp`](../NESEmulator/Cartridge.cpp)
2. [`NESEmulator/Mapper.cpp`](../NESEmulator/Mapper.cpp)
3. [`NESEmulator/Mapper_000.cpp`](../NESEmulator/Mapper_000.cpp)
4. [`NESEmulator/Mapper_001.cpp`](../NESEmulator/Mapper_001.cpp)
5. [`NESEmulator/Mapper_002.cpp`](../NESEmulator/Mapper_002.cpp)
6. [`NESEmulator/Mapper_003.cpp`](../NESEmulator/Mapper_003.cpp)
7. [`NESEmulator/Mapper_004.cpp`](../NESEmulator/Mapper_004.cpp)
8. [`NESEmulator/Mapper_066.cpp`](../NESEmulator/Mapper_066.cpp)

## 3. Execution hardware

The [controller serial helper](../NESEmulator/ControllerSerial.hpp) and [CPU test](../NESEmulator/tests/ControllerSerialTests.cpp) explain debugger read-only purity and the dual-player $4016 snapshot before reading Bus::cpuRead/cpuWrite.

1. [`NESEmulator/ControllerSerial.hpp`](../NESEmulator/ControllerSerial.hpp)
2. [`NESEmulator/Bus.cpp`](../NESEmulator/Bus.cpp)
2. [`NESEmulator/cpu6502.cpp`](../NESEmulator/cpu6502.cpp)
3. [`NESEmulator/ppu2C02.cpp`](../NESEmulator/ppu2C02.cpp)
4. [`NESEmulator/apu2A03.cpp`](../NESEmulator/apu2A03.cpp)

## 4. Host rendering and diagnostics

1. [`NESEmulator/KairoPixelGameEngine.cppm`](../NESEmulator/KairoPixelGameEngine.cppm)
2. [`NESEmulator/testing6502.cpp`](../NESEmulator/testing6502.cpp)
3. [`NESEmulator/testing2C02.cpp`](../NESEmulator/testing2C02.cpp)

## 5. Portable input regressions and CI

Read these after the cartridge/controller code and before attempting a game ROM:

1. [`NESEmulator/tests/INES1CartridgeInputTests.cpp`](../NESEmulator/tests/INES1CartridgeInputTests.cpp): header format, CHR RAM, optional trainer, truncated PRG/CHR and mapper rejection
2. [`NESEmulator/tests/ControllerSerialTests.cpp`](../NESEmulator/tests/ControllerSerialTests.cpp): both-player latch, normal serial reads and nonmutating debugger peeks
3. [`scripts/run_nes_input_regressions.sh`](../scripts/run_nes_input_regressions.sh): one-command C++23 build and execution without graphics APIs
4. [`.github/workflows/cpu-nes-input-guards.yml`](../.github/workflows/cpu-nes-input-guards.yml): GCC/Clang portable input safety checks on relevant changes

Run: `bash scripts/run_nes_input_regressions.sh`. A passing CPU test is not a full compatibility or native GPU test.

## End-to-end interaction map

`main.cpp` converts keyboard input to controller bits; `Bus.cpp` maps CPU addresses to RAM, cartridge, PPU, APU and DMA. `Cartridge.cpp` asks `Mapper_*` to translate ROM bank addresses. CPU instructions and PPU pixels advance under `Bus::clock()`. The host pixel engine displays the frame and handles input/audio. External ROM data is not source code; never distribute commercial ROMs without permission.

## APIs and code-reading questions

For each function, distinguish **contract** (preconditions, postconditions, failure modes), **implementation** (state/algorithms), **collaborators** (imports, calls and consumers), **lifetime/ownership**, and **verification** (test or evidence that checks it). For C++ modules, `export module` names the public compiled interface and `import` expresses module dependencies; an implementation unit's `module ...;` contributes definitions without exporting them.

## Boundaries and honesty

The tree includes prebuilt binaries; **two new portable input/controller CTest targets and their GitHub Actions workflow are committed**, but executed native app compatibility evidence is not established. Treat APU and mapper compatibility as work to validate, not established console-wide accuracy. Review the provenance of the adapted pixel engine and emulator components before redistribution.

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

**Historical source-index snapshot (before this continuation):** 18 code/build/test files and 21 other tracked files. Newly added and directly linked above: `INES1Header.hpp`, `ControllerSerial.hpp`, two `NESEmulator/tests` sources, `scripts/run_nes_input_regressions.sh`, the CPU input CI workflow, and the deep cartridge/controller tutorial. Rebuild the tree inventory before treating it as exhaustive.
