# Third-party attribution and license boundaries

KairoNES is shared to help people learn, modify, experiment and develop further. It includes code with **different owners and license conditions**. The root MIT license is only for original contributions for which Swayam Singal controls the rights; it does **not** automatically relicense incorporated or adapted third-party code.

## OneLoneCoder — NES tutorial and PixelGameEngine (OLC-3)

Parts of the core emulator (CPU, PPU, Bus, mappers and related files) appear to be adapted from [OneLoneCoder/olcNES](https://github.com/OneLoneCoder/olcNES) by David Barr / javidx9. `NESEmulator/KairoPixelGameEngine.cppm` appears adapted/renamed from [OneLoneCoder/olcPixelGameEngine](https://github.com/OneLoneCoder/olcPixelGameEngine). Even when identifiers are renamed, the upstream copyrights and redistribution conditions remain relevant.

**Applicable upstream license: OLC-3** (see [olcNES license](https://github.com/OneLoneCoder/olcNES#license-olc-3) and [olcPixelGameEngine license](https://github.com/OneLoneCoder/olcPixelGameEngine/blob/master/LICENCE.md)).

Copyright 2018, 2019, 2020, 2021 OneLoneCoder.com (olcNES).
Copyright 2018–2024 OneLoneCoder.com (olcPixelGameEngine, subject to the specific incorporated revision).

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions or derivations of source code must retain the above applicable copyright notice, these conditions, and the following disclaimer.
2. Redistributions or derivative works in binary form must reproduce the applicable copyright notice, these conditions and the disclaimer in accompanying documentation and/or other materials.
3. Neither the copyright holder's name nor contributors' names may be used to endorse or promote derived products without prior specific written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, LOSS OF USE, DATA, PROFITS, OR BUSINESS INTERRUPTION), HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE), ARISING FROM OR RELATED TO USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Upstream license texts take precedence if a version differs; a future provenance audit should compare the adapted files against the specific upstream commits.

## ROMs and third-party dependencies

Commercial NES game ROMs and trademarks are not licensed for redistribution by this repository. Use only game images you may legally possess and distribute. Homebrew/test ROMs also retain their creators' terms. The toolchain dependencies (LLVM, CMake, Ninja, libpng, and platform frameworks) each have independent licenses. None are relicensed by this repository.

## Original teaching additions

Independently authored teaching comments, source-reading guides, and other additions whose rights are held by Swayam Singal are available under the [MIT license](LICENSE). This statement does not override OLC-3 or any other existing third-party condition.
