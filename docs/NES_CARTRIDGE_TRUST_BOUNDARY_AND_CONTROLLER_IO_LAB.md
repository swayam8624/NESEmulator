# A game cartridge is not an array: from sixteen bytes to CPU, PPU and controller state

*A narrative C++23 hardware laboratory connecting safe iNES 1.0 input decoding, PRG/CHR banking, debugger purity, the NES bus, and the graphics frame.*

Imagine opening a game console on your workbench. You slide in a cartridge. You know the cartridge contains some program bytes, some picture-pattern bytes, and a tiny label identifying what kind of circuit board is inside. But the first thing you must **not** do is blindly trust that label. If it says there are sixteen kilobytes of program data and the cartridge supplies only three, the console emulator needs to reject it **before** the CPU begins asking the mapper for memory.

KairoNES has exactly this boundary in [`Cartridge.cpp`](../NESEmulator/Cartridge.cpp), reached when [`main.cpp`](../NESEmulator/main.cpp) creates a cartridge before calling [`Bus::insertCartridge`](../NESEmulator/Bus.cpp). This pass adds a portable header/payload reader, bounds checks, a controller-read fix, tests, and CI. The teaching path begins with those actual source files, not an invented architecture.

**Start here:** [`INES1Header.hpp`](../NESEmulator/INES1Header.hpp) → [`INES1CartridgeInputTests.cpp`](../NESEmulator/tests/INES1CartridgeInputTests.cpp) → [`Cartridge.cpp`](../NESEmulator/Cartridge.cpp) → [`Mapper_000.cpp`](../NESEmulator/Mapper_000.cpp) → [`Bus.cpp`](../NESEmulator/Bus.cpp) → [`ControllerSerial.hpp`](../NESEmulator/ControllerSerial.hpp) and [its regression](../NESEmulator/tests/ControllerSerialTests.cpp). Continue to [the prior CPU/PPU timing laboratory](CPU_6502_PPU_BUS_TIMING_LAB.md).

## 1. Three address spaces, three meanings of a number

An NES CPU address is **not** an offset into the host `std::vector` storing ROM bytes. Neither is it a direct PPU pattern-table offset.

For example, the CPU instruction `LDA $C000` reads CPU-visible address `0xC000`. The CPU calls `Bus::cpuRead(0xC000)`. Bus asks the cartridge first. The cartridge calls the selected mapper's `cpuMapRead`, which converts that address into an **offset within its own PRG memory vector**.

For the fixed-bank NROM / Mapper 000 implementation, a 16 KiB PRG cartridge mirrors that bank twice:

```text
CPU address space:            Cartridge PRG storage:
$8000 ... $BFFF ------->      offset $0000 ... $3FFF
$C000 ... $FFFF ------->      offset $0000 ... $3FFF

CPU $C000 → (0xC000 & 0x3FFF) = PRG byte offset 0x0000
CPU $FFFF → (0xFFFF & 0x3FFF) = PRG byte offset 0x3FFF
```

With 32 KiB of NROM PRG, the mapper instead uses `addr & 0x7FFF`, so `$C000` addresses offset `$4000`. This is not an arbitrary C++ optimization; it models how cartridge board address lines select ROM storage.

The PPU has its own view of cartridge CHR memory: pattern-table `$0000–$1FFF` maps to up to 8 KiB of picture-pattern storage at a time. More complex mappers change the PRG and CHR *bank selection* while the CPU/PPU-visible addresses remain stable.

The new cartridge boundary checks that every translated offset lies within `vPRGMemory` or `vCHRMemory` before dereferencing. This protects host memory even if a mutable mapper register computes an invalid bank. It is not a proof that the mapper has chosen the **hardware-correct** bank; the correctness and safety questions are separate.

## 2. The sixteen-byte iNES label

The iNES 1.0 file header occupies exactly sixteen bytes. Its first four bytes are the identifying signature:

```text
Byte 0     0x4E = 'N'
Byte 1     0x45 = 'E'
Byte 2     0x53 = 'S'
Byte 3     0x1A = historical EOF marker
Byte 4     number of 16 KiB PRG ROM banks
Byte 5     number of 8 KiB CHR ROM banks
Byte 6     flags: mirror bit, trainer bit, low mapper nibble
Byte 7     flags: high mapper nibble and format-version identification
Bytes 8–15 legacy metadata / reserved header fields
```

The new [`ParseINES1Header`](../NESEmulator/INES1Header.hpp) returns *numbers with checked meaning* rather than silently constructing PRG/CHR arrays from unchecked header characters. The supported mapper number is:

```text
mapper = (header[7] & 0xF0) | (header[6] >> 4)
```

Only mapper IDs **0, 1, 2, 3, 4, and 66** are supported by the actual KairoNES mapper classes. The helper rejects any other number **before** allocating ROM banks.

The code also rejects a broken signature, zero PRG banks, unsupported four-screen nametable mirroring and NES 2.0 format identification. The old loader tried to treat some NES 2.0 metadata as extra bank bits but stored the result in `uint8_t` fields, truncating important sizes. Rather than claim a larger cartridge works with 8-bit bank counts, the new implementation deliberately rejects NES 2.0 until extended mapper/size support is designed.

## 3. Derive real file lengths before allocating

Consider a minimal NROM file with one PRG bank and one CHR bank:

```text
16-byte header
16,384-byte PRG ROM
 8,192-byte CHR ROM
---------------------
24,592 total bytes
```

The program image describes exactly 16 KiB. The picture-pattern image describes exactly 8 KiB. If a user supplies only 8,191 CHR bytes, `ReadINES1Rom` rejects it. The previous constructor attempted to read whatever bytes were available without consistently checking the read results, and could then declare the cartridge valid.

Now consider a more surprising file: one PRG bank and **zero CHR ROM banks**.

```text
File on disk:
  16-byte header
  16,384-byte PRG ROM
  NO CHR ROM DATA                   = 16,400 bytes total

Emulator memory:
  16,384-byte PRG vector
   8,192-byte ZERO-INITIALIZED CHR RAM vector
```

There is a difference between **storage inside the emulated machine** and **bytes serialized in the ROM file**. CHR bank count zero means the game expects writable CHR RAM. The emulator must allocate that memory but must not try reading nonexistent CHR bytes from disk. This repair also makes valid CHR-RAM ROM images load without a phantom 8 KiB read.

The header's trainer bit adds another region: an optional **512-byte trainer immediately after the 16-byte header**, before PRG. For a two-bank PRG, one-bank CHR file with a trainer, the expected file layout is:

```text
16 + 512 + (2 × 16,384) + (1 × 8,192)
 = 16 + 512 + 32,768 + 8,192
 = 41,488 bytes
```

The helper requires all 512 trainer bytes if advertised, then starts reading PRG at the right offset. It rejects truncated trainer, PRG and CHR sections separately. It tolerates trailing bytes after the required payload because some legacy files have padding; those bytes are not included in the cartridge's PRG/CHR vectors.

The helper's size bound comes from the current **iNES1 8-bit bank fields**: 255 × 16,384 bytes for PRG and 255 × 8,192 bytes for CHR. Even a malicious bank count cannot make this helper request an unbounded gigabyte allocation. Other dynamic mapper state, graphic buffers and source-path security require their own limits.

## 4. Why the loader has two phases

The current source now reads the complete supported ROM before publishing a mapper:

```text
main.cpp's chosen ROM path
   |
   v
std::ifstream(sFileName)
   |
   v
ReadINES1Rom(stream)
   ├── read complete 16-byte signature and header
   ├── ParseINES1Header(header): validate format, mapper and bank counts
   ├── read optional complete 512-byte trainer
   ├── read all expected PRG bytes
   └── read all CHR ROM bytes, or allocate zeroed CHR RAM
   |
   v
Cartridge constructor (validated owned payload)
   ├── move PRG/CHR vectors to Cartridge
   ├── construct Mapper_000 / 001 / 002 / 003 / 004 / 066
   └── ImageValid = true ONLY if mapper exists
   |
   v
Bus::insertCartridge requires valid ImageValid and nonnull mapper
   ├── Bus owns shared cartridge reference for CPU reads
   └── PPU receives same cartridge for CHR/nametable behavior
```

The host's existing contract is preserved: a missing or malformed ROM leaves `ImageValid()==false` so the UI can try another path. **Unexpected allocation failures** are not silently disguised as “file missing”; they are allowed to propagate.

`Bus::insertCartridge` now explicitly rejects null, invalid or mapperless cartridges. `reset()` and `clock()` reject use before a cartridge is installed instead of dereferencing null state. CPU bus read/write can still decode internal registers without a cartridge object; those paths now check `cart` before attempting a cartridge mapping.

An important boundary remains: `ImageValid` means that this loader obtained a complete image of a supported *format and mapper*, **not** that every mapper's IRQ, CHR bank selection, PRG-ROM write policy, or cycle timing is verified against real NES hardware.

## 5. Two buses, multiple mirrors

The CPU sees 64 KiB of addresses, but this emulator owns only 2 KiB of CPU RAM and mirrors it into the first 8 KiB:

```text
CPU $0000  → RAM index 0x000
CPU $0800  → RAM index 0x000
CPU $1000  → RAM index 0x000
CPU $1800  → RAM index 0x000

index = cpuAddress & 0x07FF
```

Similarly, eight CPU-visible PPU registers repeat from `$2000` through `$3FFF`, so the decoder uses `addr & 0x0007`. This is why a debugger read at `$2008` may have the same register semantics as a read at `$2000` rather than accessing separate storage.

APU register writes in `$4000–$4013`, `$4015`, and `$4017` are decoded separately. Writing `$4014` starts a 256-byte sprite DMA. A CPU instruction does not directly copy all 256 sprite bytes itself: `Bus::clock` steals CPU slots and alternates CPU-memory reads and PPU OAM writes. PPU clocks run on each master tick; CPU slots occur at a 3:1 PPU-to-CPU ratio in the current model.

The source implements this structure, but cycle-perfect DMA alignment, APU behavior and PPU scanline/IRQ parity still need conformance tests.

## 6. A debugger is an observer, not a player

The CPU also reads the input controller serially. KairoNES holds two arrays:

```cpp
uint8_t controller[2];       // host's current button values
uint8_t controller_state[2]; // latched serial shift register bytes
```

A CPU write to `$4016` captures a snapshot of both players' bytes in this emulator's **simplified** latch model. Successive CPU reads from `$4016` or `$4017` return the most significant remaining button bit and shift that controller's latched byte left one place.

The old source had two related problems. First, its write dispatcher already routed `$4017` to the APU, meaning the later code that attempted to latch player 2 on `$4017` was unreachable. The new [`LatchControllerPair`](../NESEmulator/ControllerSerial.hpp) is invoked by the `$4016` write path and captures both players consistently. This still does **not** implement the exact high-strobe/falling-edge controller protocol of original hardware; that is a separate improvement.

Second, `Bus::cpuRead(address,true)` is used by a debugger to *peek*. For an input controller, a normal read consumes a bit, but a debug peek must not consume it. Previously, both reads shifted the byte.

The new [`ReadControllerSerialBit`](../NESEmulator/ControllerSerial.hpp) makes that distinction explicit:

```cpp
uint8_t ReadControllerSerialBit(uint8_t& latched, bool readOnly) {
    uint8_t bit = (latched & 0x80) != 0 ? 1 : 0;
    if (!readOnly)
        latched = uint8_t(latched << 1);
    return bit;
}
```

Suppose player 1's latched byte is binary `10110000`:

| Operation | Returned bit | Latch after |
| --- | ---: | --- |
| Debug peek | 1 | `10110000` |
| Debug peek again | 1 | `10110000` |
| CPU read | 1 | `01100000` |
| Debug peek | 0 | `01100000` |
| CPU read | 0 | `11000000` |
| CPU read | 1 | `10000000` |
| CPU read | 1 | `00000000` |

The important part is not whether the debugger can render a nice hexadecimal display. It is that **observing the machine does not alter what the next real CPU instruction sees**. This is a common principle in emulators, debuggers, profilers and distributed tracing systems.

The code still shifts zeros after consuming all eight bits, whereas real NES controller implementations have additional post-eight-bit behavior. Likewise, bReadOnly does not automatically make every possible mapper/APU/PPU register side-effect-free. The fix is scoped specifically to the controller path that was demonstrably wrong.

## 7. What the new tests prove, and what they do not

There are now two entirely portable, C++23 regression programs:

| Test | Source-backed verification |
| --- | --- |
| [`INES1CartridgeInputTests.cpp`](../NESEmulator/tests/INES1CartridgeInputTests.cpp) | Signature, 16-/32-KiB PRG, CHR ROM vs zeroed CHR RAM, mapper 66, trainer offset, highest legacy bank counts, unsupported mapper/version/four-screen rejection, and incomplete input |
| [`ControllerSerialTests.cpp`](../NESEmulator/tests/ControllerSerialTests.cpp) | Two-player latch snapshot, interleaved debugger peeks without shift, actual CPU serial-bit consumption, player-stream independence |

They do **not** invoke PixelGameEngine, display commercial ROM data, initialize a GPU, prove actual mapper IRQ timing, or compare 6502 cycles against a hardware capture. The test bytes are constructed in source and carry no game IP.

Both tests are registered through [`CMakeLists.txt`](../CMakeLists.txt) as `KairoNESCartridgeInputTests` and `KairoNESControllerSerialTests` when `BUILD_TESTING=ON`. They can also run independently from the macOS game build:

```bash
# From the NESEmulator repository root:
bash scripts/run_nes_input_regressions.sh

# Or select a particular compiler:
CXX=clang++ bash scripts/run_nes_input_regressions.sh
```

The [portable GCC/Clang CI workflow](../.github/workflows/cpu-nes-input-guards.yml) invokes the same standalone script on relevant pushes and pull requests. **A committed workflow file is not itself evidence that GitHub Actions jobs have completed successfully**; exact-head job status and native app builds must be checked separately.

## 8. Exercises that connect the learning to real code

A strong next exercise is to build a 16-byte header yourself, generate 16 KiB of predictable program bytes (for example each byte `i & 0xFF`), decode it through `ReadINES1Rom`, and then trace how Mapper 000 turns CPU address `$C123` into PRG offset `$0123` for a one-bank NROM file. The resulting value should be the generated PRG byte `0x23`. Change the header to two PRG banks and watch the same CPU address now resolve to `$4123`.

Then construct a CHR-RAM image with zero CHR ROM bytes. Confirm `vCHRMemory` begins as 8 KiB of zeroes, and that mapper-permitted PPU writes can change those bytes. Compare that with CHR-ROM mapping, which should not accidentally write to read-only pattern data.

Finally, trace a normal CPU controller read surrounded by multiple debugger peeks. The *observed* bit sequence should be independent of the number of peeks. For real hardware compatibility, expand the controller model to implement strobe-high and falling-edge latching and measure post-eight-bit reads against a trusted emulator/hardware trace—do not silently assert that the existing simplified register model is complete.

## 9. Next compatibility and security gates

This pass closes the most direct unsafe iNES-header and unbounded/truncated-memory indexing paths, but full untrusted-file safety takes more than one check.

Remaining work includes NES 2.0 extended mapper and bank encoding; four-screen nametable mirroring; mapper-specific PRG-RAM and bank-switch verification; NROM PRG-ROM write behavior; controller strobe-edge behavior; read-only semantics of PPU/APU/mappers; bounded time/complexity of more elaborate media importers; interrupt/scanline parity; APU fidelity; and native app compilation plus conformance tests on legally obtained fixtures.

The current renderer remains macOS-oriented and relies on an adapted PixelGameEngine component. The project's open-source license covers only the original contributions for which the author holds rights. A clean build or passing CPU safety regression must **not** be mistaken for permission to redistribute commercial ROMs or third-party source/assets.

The engineering lesson generalizes far beyond the NES: **parse before allocating, map before indexing, validate before publishing, and separate inspection from mutation**. This is the same ownership and fault-recovery discipline used in GPU resource managers and real-time rendering systems.
