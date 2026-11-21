# KairoNES

KairoNES is a C++23 NES emulator project built around C++ modules. The emulator core is exposed through `NESEmulator/KairoNES.cppm`, with implementation units in `.cpp` files.

## Learn the source

**Start with [the linked source reading order](docs/SOURCE_READING_ORDER.md)**. It explains the recommended file sequence, cross-module call paths, mapper families, PPU/CPU interactions, and focused exercises. Read [provenance and ROM redistribution notes](docs/THIRD_PARTY_AND_ROMS.md) before reusing or shipping bundled code or assets.

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
./build/game "build/roms/local/Super Mario Bros. 3 (USA) (Rev 1).nes"
```

```bash
./build/game "build/roms/local/Legend of Zelda, The (USA) (Rev 1).nes"
```

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

## Licensing and third-party components

This project is published as source for study, but the repository currently contains bundled dependencies and ROM assets whose rights must be evaluated separately. Do not assume a blanket open-source license applies to the entire tree. See [the component/provenance guidance](docs/THIRD_PARTY_AND_ROMS.md). The new educational guides are separately licensed in [LICENSE-DOCS](LICENSE-DOCS).

## Notes

- The active build target is `game`.
- The emulator currently starts in running mode.
- Audio is not the frame driver; emulation advances from the application update loop.
