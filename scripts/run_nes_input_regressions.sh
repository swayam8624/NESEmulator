#!/usr/bin/env bash
# Build and execute the portable C++23 iNES file-safety and controller
# register-side-effect regressions. No macOS OpenGL, Homebrew, game executable,
# third-party ROMs, emulator modules, or game-window setup is required.
#
# From the NESEmulator repository root:
#   bash scripts/run_nes_input_regressions.sh
#   CXX=clang++ bash scripts/run_nes_input_regressions.sh
#
# These tests DO NOT establish full 6502/PPU cycle accuracy or NES 2.0 support.
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "$script_dir/.." && pwd)"
compiler="${CXX:-c++}"
work="$(mktemp -d "${TMPDIR:-/tmp}/kairo-nes-input.XXXXXXXX")"
trap 'rm -rf -- "$work"' EXIT

if ! command -v "$compiler" >/dev/null 2>&1; then
  echo "C++23 compiler not found: $compiler" >&2
  exit 1
fi

printf '%s\n' '=== Compiler ==='
"$compiler" --version | head -n 1 || true

printf '%s\n' '=== iNES1 header, trainer, PRG, CHR ROM/RAM and truncation ==='
"$compiler" -std=c++23 -O0 -g -Wall -Wextra -Werror -pedantic \
  "$repo_root/NESEmulator/tests/INES1CartridgeInputTests.cpp" \
  -o "$work/kairo-ines1-input"
"$work/kairo-ines1-input"

printf '%s\n' '=== CPU controller shift and read-only debugger inspection ==='
"$compiler" -std=c++23 -O0 -g -Wall -Wextra -Werror -pedantic \
  "$repo_root/NESEmulator/tests/ControllerSerialTests.cpp" \
  -o "$work/kairo-controller-serial"
"$work/kairo-controller-serial"

printf '%s\n' 'PASS: both portable C++23 iNES input regressions completed.'
printf '%s\n' 'NATIVE ROM GAMEPLAY and 6502/PPU cycle parity need separate testing.'
