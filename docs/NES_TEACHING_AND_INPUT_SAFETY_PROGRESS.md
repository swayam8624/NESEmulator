# KairoNES learning and input-safety progress — 23 November 2026

This is an inspectable record of **actual changes committed on the active main branch**. It is not a statement of full NES compatibility, executed Mac native builds or already-passed GitHub Actions.

## Two complementary teaching chapters

The [cartridge trust boundary and two-controller I/O laboratory](NES_CARTRIDGE_TRUST_BOUNDARY_AND_CONTROLLER_IO_LAB.md) follows iNES header bytes, supported mapper IDs, PRG/CHR payload lengths, trainer offset, CHR-RAM semantics, and read-only controller inspection. The [one instruction to one PPU raster pixel laboratory](NES_ONE_INSTRUCTION_TO_RASTER_LAB.md) follows NROM reset vectors, opcode callbacks, ADC/SBC flags, page crossing, PPU scroll/address latches, pattern shifters, sprite priority, vblank/NMI and MMC3 IRQ approximation.

These are linked in [README](../README.md), [source reading order](SOURCE_READING_ORDER.md), [architecture guide](NES_ARCHITECTURE_GUIDE.md), and [existing bus/flags lesson](CPU_6502_PPU_BUS_TIMING_LAB.md).

## Production source and new regression coverage

| Source | Completed source change | Contract |
| --- | --- | --- |
| [`INES1Header.hpp`](../NESEmulator/INES1Header.hpp) | New header/payload validator and reader | Correct iNES1 signature; limited mapper whitelist; reject NES2 and four-screen; complete optional trainer/PRG/CHR; zero-bank CHR means 8 KiB zeroed RAM, not extra file bytes |
| [`Cartridge.cpp`](../NESEmulator/Cartridge.cpp) | Replaced unvalidated stream reads; constrained mapper publication; checked mapped indices | No `ImageValid` until complete supported ROM and mapper; PRG/CHR offsets checked before vector access |
| [`Bus.cpp`](../NESEmulator/Bus.cpp) | Valid cartridge preconditions, guarded cartridge dispatch, controller latch + debug peek semantics | Reset/clock reject missing cartridge; $4016 captures both controllers; CPU reads shift, readonly peeks do not |
| [`ControllerSerial.hpp`](../NESEmulator/ControllerSerial.hpp) | New pure C++ serial/latch helpers | Separates CPU read side effect from debugger observation |
| [`cpu6502.cpp`](../NESEmulator/cpu6502.cpp) | 13 new worked implementation comments | Reset vector, IRQ/NMI stack, addressing, carry/overflow, signed branches, indirect JMP quirk |
| [`ppu2C02.cpp`](../NESEmulator/ppu2C02.cpp) | 14 new worked implementation comments | $2002/$2005/$2006/$2007 semantics, PPU address mirrors, fine/coarse scroll, pixel shifters, sprite priority, vblank |
| [`Mapper_004.cpp`](../NESEmulator/Mapper_004.cpp) | 6 new worked implementation comments | PRG/CHR bank window mapping, MMC3 register writes, approximate IRQ reload |

The source changes were limited to focused safety/behavior corrections and explanatory comments. The underlying emulator timing and rendering algorithms were not replaced by an invented redesign.

### Portable CPU-only regressions

The [iNES header/payload regression](../NESEmulator/tests/INES1CartridgeInputTests.cpp) uses **entirely generated fixture bytes**: correct/invalid magic, unsupported mapper, NES2/4-screen rejection, trainer truncation, PRG/CHR truncation, 8 KiB CHR RAM, 16-/32-KiB PRG and mapper 66.

The [controller regression](../NESEmulator/tests/ControllerSerialTests.cpp) checks both-player latch snapshot, normal serial consumption, readonly peeks preserving the shift state, and player isolation.

These are registered in [CMake](../CMakeLists.txt) as `KairoNESCartridgeInputTests` and `KairoNESControllerSerialTests`. The [standalone CPU runner](../scripts/run_nes_input_regressions.sh) avoids the macOS game’s Homebrew/OpenGL dependency:

```bash
bash scripts/run_nes_input_regressions.sh
```

The [GCC/Clang CI workflow](../.github/workflows/cpu-nes-input-guards.yml) runs that script on relevant pushes and pull requests. **Committing the script/workflow and reviewing assertions do not count as executing these exact programs.**

## Validation gaps that remain

1. **Native app build:** CMake 4/C++23 modules, the adapted PixelGameEngine, Homebrew dependencies and OpenGL/macOS runtime have not been rebuilt from this exact commit here.
2. **NES CPU compatibility:** no reference-trace comparison for official 6502 opcode/addressing/interrupt/cycle behavior. `cpu6502::clock` performs operations at instruction start and counts down; not cycle-by-cycle bus micro-ops.
3. **PPU fidelity:** register latch, palette buffering, scrolling, sprite-zero, odd-frame timing and 256×240 reference raster comparisons still need an independent oracle.
4. **Input controllers:** $4016 snapshot is a simplified model; exact physical strobe-high and falling-edge behavior and post-eight-bit reads are not implemented.
5. **Mapper behavior:** NROM PRG-ROM write policy, mapper 001/002/003/004/066 bank switching, PRG-RAM, MMC3 A12-qualified IRQ timing and boundary cases need targeted fixtures.
6. **APU:** sound timing and channel behavior are partial; native output should be measured rather than inferred from seeing frames.
7. **Other ROM formats:** NES 2.0 wider banks/mappers and four-screen mirroring are intentionally rejected, not silently treated as iNES 1.0.
8. **Licensing:** top-level MIT grant applies to original contributions only; third-party adapted source and game ROMs retain their own rights.

## Recommended next engineering experiment

Implement a **portable headless synthetic-NROM instruction runner** (with no window or copyrighted ROMs) that assembles byte sequences `A9 7F 69 01`, vectors reset to `$8000`, steps until each instruction completes and records PC/A/X/Y/SP/P flags and cycle counts. Then compare every computed step to an independently checked 6502 oracle. A separate PPU fixture should drive $2005/$2006/$2007 and compare observed scroll/address latches and palette/mirroring responses.

Only after those tests exist should visible pixels or the number of games booting be treated as compatibility evidence. The source-tracing chapters already explain which components, state variables and interfaces the test must instrument.
