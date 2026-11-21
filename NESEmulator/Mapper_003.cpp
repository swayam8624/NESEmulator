// TEACHING FILE GUIDE
// CNROM-style CHR switching. CPU writes choose which graphics bank the PPU
// can read through Cartridge. PRG addressing remains simpler than MMC
// families. Rendering artifacts may arise from wrong PPU CHR bank selection
// rather than a bug in ppu2C02 itself.
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
// TEACHING API: Mapper_003::Mapper_003
// Initialize mapper bank counts; CHR selection starts in its reset state.
// Integration: CNROM: fixed PRG mapping with a CPU-controlled switchable CHR graphics bank.
Mapper_003::Mapper_003(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks)
{
}


// TEACHING API: Mapper_003::~Mapper_003
// Cartridge currently stores make_shared<Derived> objects as shared_ptr<Mapper>;
// the shared control block retains the concrete deleter. Mapper's base
// destructor is not virtual, so deleting a derived object through a raw
// Mapper* would be unsafe. Mapper instances do not own PRG/CHR ROM bytes.
Mapper_003::~Mapper_003()
{
}

// TEACHING API: Mapper_003::cpuMapRead
// Map 16 KiB PRG with mirroring or 32 KiB PRG directly; other bank counts need explicit validation.
// Integration: CNROM: fixed PRG mapping with a CPU-controlled switchable CHR graphics bank.
bool Mapper_003::cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data)
{
	if (addr >= 0x8000 && addr <= 0xFFFF)
	{
		if (nPRGBanks == 1) // 16K ROM 
			mapped_addr = addr & 0x3FFF;
		if (nPRGBanks == 2) // 32K ROM
			mapped_addr = addr & 0x7FFF;
		return true;
	}
	else
		return false;
}

// TEACHING API: Mapper_003::cpuMapWrite
// Latch low CHR bank-select bits from writes to the cartridge register range; avoid mutating PRG ROM.
// Integration: CNROM: fixed PRG mapping with a CPU-controlled switchable CHR graphics bank.
bool Mapper_003::cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data)
{
	if (addr >= 0x8000 && addr <= 0xFFFF)
	{
		nCHRBankSelect = data & 0x03;
		mapped_addr = addr;		
	}

	// Mapper has handled write, but do not update ROMs
	return false;
}

// TEACHING API: Mapper_003::ppuMapRead
// Offset a PPU pattern-table read into the selected 8 KiB CHR bank.
// Integration: CNROM: fixed PRG mapping with a CPU-controlled switchable CHR graphics bank.
bool Mapper_003::ppuMapRead(uint16_t addr, uint32_t &mapped_addr)
{
	if (addr < 0x2000)
	{
		mapped_addr = nCHRBankSelect * 0x2000 + addr;
		return true;
	}
	else
		return false;
}

// TEACHING API: Mapper_003::ppuMapWrite
// Return unhandled for CHR ROM; no writable CHR path is implemented here.
// Integration: CNROM: fixed PRG mapping with a CPU-controlled switchable CHR graphics bank.
bool Mapper_003::ppuMapWrite(uint16_t addr, uint32_t &mapped_addr)
{
	return false;
}

// TEACHING API: Mapper_003::reset
// Return CHR selector to the initial bank.
// Integration: CNROM: fixed PRG mapping with a CPU-controlled switchable CHR graphics bank.
void Mapper_003::reset()
{
	nCHRBankSelect = 0;
}
