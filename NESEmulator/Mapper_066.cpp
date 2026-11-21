// TEACHING FILE GUIDE
// GxROM-style PRG/CHR bank selection. A single selection register maps
// distinct code and pattern banks. Cartridge still performs byte access;
// this module supplies mapping arithmetic only. Test boundary bank values
// and ensure resulting PRG/CHR indices remain inside allocated memory.
// See docs/SOURCE_READING_ORDER.md and the companion module declarations.

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
// TEACHING API: Mapper_066::Mapper_066
// Initialize PRG/CHR bank counts inherited from Mapper.
// Integration: GxROM: CPU control register selects both a 32 KiB PRG window and an 8 KiB CHR bank.
Mapper_066::Mapper_066(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks)
{
}


// TEACHING API: Mapper_066::~Mapper_066
// Cartridge currently stores make_shared<Derived> objects as shared_ptr<Mapper>;
// the shared control block retains the concrete deleter. Mapper's base
// destructor is not virtual, so deleting a derived object through a raw
// Mapper* would be unsafe. Mapper instances do not own PRG/CHR ROM bytes.
Mapper_066::~Mapper_066()
{
}

// TEACHING API: Mapper_066::cpuMapRead
// Map CPU $8000-$FFFF through the selected 32 KiB PRG ROM bank.
// Integration: GxROM: CPU control register selects both a 32 KiB PRG window and an 8 KiB CHR bank.
bool Mapper_066::cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data)
{
	if (addr >= 0x8000 && addr <= 0xFFFF)
	{
		mapped_addr = nPRGBankSelect * 0x8000 + (addr & 0x7FFF);
		return true;
	}
	else
		return false;
}

// TEACHING API: Mapper_066::cpuMapWrite
// Decode CHR bank from low bits and PRG bank from high bits; this is a bank command, not a ROM data write.
// Integration: GxROM: CPU control register selects both a 32 KiB PRG window and an 8 KiB CHR bank.
bool Mapper_066::cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data)
{
	if (addr >= 0x8000 && addr <= 0xFFFF)
	{
		nCHRBankSelect = data & 0x03;
		nPRGBankSelect = (data & 0x30) >> 4;
	}
	
	// Mapper has handled write, but do not update ROMs
	return false;
}

// TEACHING API: Mapper_066::ppuMapRead
// Map pattern-table bytes into the selected 8 KiB CHR bank.
// Integration: GxROM: CPU control register selects both a 32 KiB PRG window and an 8 KiB CHR bank.
bool Mapper_066::ppuMapRead(uint16_t addr, uint32_t &mapped_addr)
{
	if (addr < 0x2000)
	{
		mapped_addr = nCHRBankSelect * 0x2000 + addr;
		return true;
	}
	else
		return false;
}

// TEACHING API: Mapper_066::ppuMapWrite
// No writable pattern-memory mapping is implemented for this mapper.
// Integration: GxROM: CPU control register selects both a 32 KiB PRG window and an 8 KiB CHR bank.
bool Mapper_066::ppuMapWrite(uint16_t addr, uint32_t &mapped_addr)
{
	return false;
}

// TEACHING API: Mapper_066::reset
// Restore CHR/PRG selectors to zero.
// Integration: GxROM: CPU control register selects both a 32 KiB PRG window and an 8 KiB CHR bank.
void Mapper_066::reset()
{
	nCHRBankSelect = 0;
	nPRGBankSelect = 0;
}
