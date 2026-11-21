# NES emulator provenance and third-party material

This repository is intended to be learnable and extensible, but **ownership and
license provenance are not yet fully audited**. The root LICENSE grants MIT
rights only for original contributions controlled by its stated copyright
holder. It does not override any upstream license.

## Items requiring a source-level provenance review

- `NESEmulator/KairoPixelGameEngine.cppm`: inspect its embedded pixel-engine,
  platform and dependency sections for upstream authors, modifications,
  notices and redistribution conditions. Do not assume that renaming APIs
  changes source provenance.
- `NESEmulator/cpu6502.cpp`, `ppu2C02.cpp`, `Bus.cpp`,
  `Cartridge.cpp`, and `Mapper_*.cpp`: compare against the original
  implementations or tutorials from which this code may have been adapted,
  preserve upstream credit/license, and document exact modifications.
- `NESEmulator/nestest.nes`, `NESEmulator/roms/`, `game` and `ppu_demo`:
  verify redistribution rights for test ROMs and generated binaries separately.
  Do not add commercial ROMs to a public distribution.

## Before redistribution

1. Identify upstream source, original authors, the exact upstream license and
   any copyright notices for each borrowed portion.
2. Keep required licenses and attribution in the repository and, when required,
   binary distributions.
3. Remove or replace material with unclear redistribution permission.
4. Record verified attribution here with exact source links and versions.

This document intentionally avoids inventing upstream attributions or claiming
ownership over preexisting third-party work.
