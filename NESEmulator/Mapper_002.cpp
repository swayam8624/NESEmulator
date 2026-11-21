// TEACHING FILE GUIDE
// UxROM-style PRG bank selector. CPU writes update a switchable code bank
// while a fixed bank remains available for vectors/startup logic. Cartridge
// owns ROM bytes; this mapper chooses offsets for Bus CPU memory accesses.
// Compare reset bank and boundaries against Mapper_000 and mapper tests.
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
// TEACHING API: Mapper_002::Mapper_002
// Initialize bank selector state through the base class and subsequent reset.
// Integration: UxROM: switchable lower PRG window, fixed upper PRG window, generally simple CHR mapping.
Mapper_002::Mapper_002(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks)
{
}


// TEACHING API: Mapper_002::~Mapper_002
// Destruction follows virtual mapper ownership in Cartridge; this class does not own PRG/CHR ROM buffers.
Mapper_002::~Mapper_002()
{
}

// TEACHING API: Mapper_002::cpuMapRead
// Use selected 16 KiB PRG bank for $8000-$BFFF; use fixed high bank for $C000-$FFFF.
// Integration: UxROM: switchable lower PRG window, fixed upper PRG window, generally simple CHR mapping.
bool Mapper_002::cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data)
{
	if (addr >= 0x8000 && addr <= 0xBFFF)
	{
		mapped_addr = nPRGBankSelectLo * 0x4000 + (addr & 0x3FFF);
		return true;
	}

	if (addr >= 0xC000 && addr <= 0xFFFF)
	{
		mapped_addr = nPRGBankSelectHi * 0x4000 + (addr & 0x3FFF);
		return true;
	}
	
	return false;
}

// TEACHING API: Mapper_002::cpuMapWrite
// Treat writes in $8000-$FFFF as bank-select commands, not as writes to immutable ROM bytes.
// Integration: UxROM: switchable lower PRG window, fixed upper PRG window, generally simple CHR mapping.
bool Mapper_002::cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data)
{
	if (addr >= 0x8000 && addr <= 0xFFFF)
	{		
		nPRGBankSelectLo = data & 0x0F;
	}

	// Mapper has handled write, but do not update ROMs
	return false;
}

// TEACHING API: Mapper_002::ppuMapRead
// Map pattern addresses directly into CHR memory; Cartridge performs the read.
// Integration: UxROM: switchable lower PRG window, fixed upper PRG window, generally simple CHR mapping.
bool Mapper_002::ppuMapRead(uint16_t addr, uint32_t &mapped_addr)
{
	if (addr < 0x2000)
	{
		mapped_addr = addr;
		return true;
	}
	else
		return false;
}

// TEACHING API: Mapper_002::ppuMapWrite
// Permit CHR writes only for supported RAM-backed cartridges.
// Integration: UxROM: switchable lower PRG window, fixed upper PRG window, generally simple CHR mapping.
bool Mapper_002::ppuMapWrite(uint16_t addr, uint32_t &mapped_addr)
{
	if (addr < 0x2000)
	{
		if (nCHRBanks == 0) // Treating as RAM
		{
			mapped_addr = addr;
			return true;
		}
	}
	return false;
}

// TEACHING API: Mapper_002::reset
// Restore lower/upper bank selectors to initial configuration.
// Integration: UxROM: switchable lower PRG window, fixed upper PRG window, generally simple CHR mapping.
void Mapper_002::reset()
{
	nPRGBankSelectLo = 0;
	nPRGBankSelectHi = nPRGBanks - 1;
}
