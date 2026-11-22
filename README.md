# KairoNES

**Learn the implementation:** [NES architecture and hardware walkthrough](docs/NES_ARCHITECTURE_GUIDE.md) · [Source reading order](docs/SOURCE_READING_ORDER.md) · [6502/PPU bus timing and CPU flags laboratory](docs/CPU_6502_PPU_BUS_TIMING_LAB.md)

KairoNES is a C++23 NES emulator project built around C++ modules. The emulator core is exposed through `NESEmulator/KairoNES.cppm`, with implementation units in `.cpp` files.

## Requirements

- macOS
- CMake 4.0 or newer
- Ninja
- Homebrew LLVM with `clang++` and `clang-scan-deps`
- Homebrew `libpng`
- Xcode command line tools

Install the common dependencies:

```bash
brew install cmake ninja llvm libpng
```

## Build

From the repository root:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
  -DCMAKE_CXX_COMPILER_CLANG_SCAN_DEPS=/opt/homebrew/opt/llvm/bin/clang-scan-deps \
  -DCMAKE_OSX_SYSROOT=$(xcrun --show-sdk-path)

cmake --build build
```

The build copies `NESEmulator/nestest.nes` and `NESEmulator/roms/` into `build/`.

## Run

Run any iNES `.nes` file by passing its path:

```bash
# Replace with your own legally obtained and compatible ROM image.
./build/game "build/roms/local/my-legally-obtained-game.nes"
```

**Input safety limitation:** the cartridge loader currently needs additional
iNES header, payload-length and unsupported-mapper validation. Do not treat
arbitrary downloaded ROM files as safe inputs until that work is completed.

Homebrew/test ROM examples:

```bash
./build/game "build/roms/bingo/bingo.nes"
./build/game "build/roms/bombsweeper/BombSweeper.nes"
```

## Controls

| NES input | Keyboard |
| --- | --- |
| D-pad | Arrow keys or WASD |
| A | J or K |
| B | L |
| Select | Space |
| Start | Enter |
| Pause/resume emulator | P |
| Reset | R |

## ROMs

Place your own legally dumped `.nes` files under:

```text
NESEmulator/roms/local/
```

Then rebuild or copy `NESEmulator/roms` into `build/roms`.

This repository should not be used to distribute copyrighted commercial ROMs unless you have the legal right to do so.

## Project Layout

```text
NESEmulator/KairoNES.cppm              Public emulator module interface
NESEmulator/KairoPixelGameEngine.cppm  Kairo pixel engine single-file dependency
NESEmulator/Bus.cpp                    NES bus, DMA, controllers, clocking
NESEmulator/cpu6502.cpp                6502 CPU implementation
NESEmulator/ppu2C02.cpp                PPU rendering implementation
NESEmulator/apu2A03.cpp                APU implementation stub/basic support
NESEmulator/Cartridge.cpp              iNES cartridge loader
NESEmulator/Mapper_*.cpp               Mapper implementations
NESEmulator/main.cpp                   App entry point and keyboard controls
```

## Notes

- The active build target is `game`.
- The emulator currently starts in running mode.
- Audio is not the frame driver; emulation advances from the application update loop.


## Learn this repository from source

The [Teaching source map](docs/SOURCE_READING_ORDER.md) gives a linked reading sequence, subsystem interactions, public API boundaries, and verification questions. Begin with the public contract and entry point, trace one real operation through its implementation, and use the linked tests to distinguish expected behavior from observed evidence.

**Current scope and limitations.** This is an educational emulator implementation, not a certification of NES compatibility. The current APU is partial and no automated CTest/Actions verification is published. Obtain ROMs legally. The emulator includes adapted/third-party material; a repository license cannot override upstream copyright.

**Learning and reuse.** Project-authored code is intended for study, modification and further development under the repository's applicable license. See [LICENSE](LICENSE) where present. Third-party libraries, borrowed source, assets, test data, models and media remain under their own terms; do not treat a top-level license as permission to redistribute restricted content.
