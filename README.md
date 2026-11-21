# KairoNES

> **Start learning here:** [READING_ORDER.md](READING_ORDER.md) gives a linked, step-by-step tour of every emulator source file, the chip/host call graph, CPU-versus-PPU addresses, clock scheduling, and an API glossary. Source comments link each subsystem to its collaborators.
>
> **Open source for learning and development:** independently owned contributions are [MIT-licensed](LICENSE), while OneLoneCoder-derived emulator and PixelGameEngine material retains its existing **OLC-3** terms. Read [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) before reuse, modification, or binary redistribution. Improvements are welcome under [CONTRIBUTING.md](CONTRIBUTING.md).
>
> **Implementation limitations:** the APU is partial, the active CMake target has no automated conformance tests, and the current cartridge loader needs stronger truncated/unsupported ROM rejection. Visual demos are not evidence of cycle-accurate emulation.


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

## Notes

- The active build target is `game`.
- The emulator currently starts in running mode.
- Audio is not the frame driver; emulation advances from the application update loop.
