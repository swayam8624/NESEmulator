# Contributing to KairoNES

Contributions that make the emulator more **correct, understandable, and reproducible** are welcome.

Read [READING_ORDER.md](READING_ORDER.md) before adding or changing chip logic. Keep comments focused on **why a design exists, its invariants, API preconditions, timebase, ownership, and cross-file calls**, rather than narrating syntax. A PPU address is not a CPU address; a host frame is not an NES hardware tick.

For functional changes, explain the observed behavior, provide a minimal reproducible ROM fixture with redistribution rights, add deterministic regression coverage (CPU instruction/flag/cycle, PPU timing/scanline, mapper banking/IRQ, APU sampling), and document known limitations. State whether you have locally built with CMake/Clang on macOS; CI is not currently set up. Never describe an unexecuted test as passing.

For docs-only changes, keep links valid and ensure claims match actual implementations. For code borrowed from upstream projects, preserve authors, notices, third-party licenses and source provenance. Original contributions are offered under MIT only to the extent that the contributor has the rights; see [LICENSE](LICENSE) and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
