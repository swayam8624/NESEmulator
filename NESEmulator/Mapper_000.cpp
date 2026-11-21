// TEACHING FILE GUIDE
// NROM baseline mapper: a fixed PRG and CHR arrangement, useful for learning
// before bank-switching mappers. Cartridge owns the byte arrays; this mapper
// returns offsets for the Bus/PPU to access. Mirroring behavior affects PPU
// nametables, while PRG bank count affects CPU address mirroring.
// Compare emulator output with an NROM test ROM before adding other mappers.
// Further reading: docs/SOURCE_READING_ORDER.md

module;
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "KairoPixelGameEngine.cppm"
module Kairo.NES;
// TEACHING API: Mapper_000::Mapper_000
// NROM has no bank-select registers: store ROM bank counts in the base Mapper.
// Integration: NROM: fixed CPU PRG mapping and CHR mapping. Cartridge owns bytes; this class computes indices.
Mapper_000::Mapper_000(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks)
{
}


// TEACHING API: Mapper_000::~Mapper_000
// Destruction follows virtual mapper ownership in Cartridge; this class does not own PRG/CHR ROM buffers.
Mapper_000::~Mapper_000()
{
}

// TEACHING API: Mapper_000::reset
// Fixed mapping requires no mutable bank-select state at reset.
// Integration: NROM: fixed CPU PRG mapping and CHR mapping. Cartridge owns bytes; this class computes indices.
void Mapper_000::reset()
{

}

// TEACHING API: Mapper_000::cpuMapRead
// Map CPU $8000-$FFFF to one mirrored 16 KiB PRG bank or two contiguous 16 KiB banks.
// Integration: NROM: fixed CPU PRG mapping and CHR mapping. Cartridge owns bytes; this class computes indices.
bool Mapper_000::cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data)
{
	// if PRGROM is 16KB
	//     CPU Address Bus          PRG ROM
	//     0x8000 -> 0xBFFF: Map    0x0000 -> 0x3FFF
	//     0xC000 -> 0xFFFF: Mirror 0x0000 -> 0x3FFF
	// if PRGROM is 32KB
	//     CPU Address Bus          PRG ROM
	//     0x8000 -> 0xFFFF: Map    0x0000 -> 0x7FFF	
	if (addr >= 0x8000 && addr <= 0xFFFF)
	{
		mapped_addr = addr & (nPRGBanks > 1 ? 0x7FFF : 0x3FFF);
		return true;
	}

	return false;
}

// TEACHING API: Mapper_000::cpuMapWrite
// This implementation maps writes to PRG offsets even though NROM PRG is usually read-only; audit this against hardware before claiming compatibility.
// Integration: NROM: fixed CPU PRG mapping and CHR mapping. Cartridge owns bytes; this class computes indices.
bool Mapper_000::cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data)
{
	if (addr >= 0x8000 && addr <= 0xFFFF)
	{
		mapped_addr = addr & (nPRGBanks > 1 ? 0x7FFF : 0x3FFF);
		return true;
	}

	return false;
}

// TEACHING API: Mapper_000::ppuMapRead
// Map PPU pattern-table addresses to CHR storage. Cartridge performs the actual byte read.
// Integration: NROM: fixed CPU PRG mapping and CHR mapping. Cartridge owns bytes; this class computes indices.
bool Mapper_000::ppuMapRead(uint16_t addr, uint32_t &mapped_addr)
{
	// There is no mapping required for PPU
	// PPU Address Bus          CHR ROM
	// 0x0000 -> 0x1FFF: Map    0x0000 -> 0x1FFF
	if (addr >= 0x0000 && addr <= 0x1FFF)
	{
		mapped_addr = addr;
		return true;
	}

	return false;
}

// TEACHING API: Mapper_000::ppuMapWrite
// Only allow pattern-table writes with CHR RAM; CHR ROM must remain read-only.
// Integration: NROM: fixed CPU PRG mapping and CHR mapping. Cartridge owns bytes; this class computes indices.
bool Mapper_000::ppuMapWrite(uint16_t addr, uint32_t &mapped_addr)
{
	if (addr >= 0x0000 && addr <= 0x1FFF)
	{
		if (nCHRBanks == 0)
		{
			// Treat as RAM
			mapped_addr = addr;
			return true;
		}
	}

	return false;
}

