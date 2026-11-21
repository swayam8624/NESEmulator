// TEACHING FILE GUIDE
// Abstract mapper defaults shared by concrete Mapper_000/001/002/003/004/066.
// Cartridge creates the appropriate mapping object from ROM header mapper ID;
// Bus/PPU reads and writes consult it for PRG/CHR bank translation.
// Mapper IRQ, mirroring and scanline hooks are hardware-specific extension
// points. Returning unhandled address must be distinct from returning a bad
// offset into PRG or CHR memory.
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
// TEACHING API: Mapper::Mapper
// Store the PRG/CHR bank counts read from the ROM; subclasses use these to calculate valid bank offsets.
// Integration: Base mapper: defines overridable CPU/PPU address mapping, mirroring and IRQ hooks used by Cartridge and Bus.
Mapper::Mapper(uint8_t prgBanks, uint8_t chrBanks)
{
	nPRGBanks = prgBanks;
	nCHRBanks = chrBanks;
}


// TEACHING API: Mapper::~Mapper
// Destruction follows virtual mapper ownership in Cartridge; this class does not own PRG/CHR ROM buffers.
Mapper::~Mapper()
{
}

// TEACHING API: Mapper::reset
// Default no-op; subclasses replace it when mapper registers must return to a power-on configuration.
// Integration: Base mapper: defines overridable CPU/PPU address mapping, mirroring and IRQ hooks used by Cartridge and Bus.
void Mapper::reset()
{

}

// TEACHING API: Mapper::mirror
// Return HARDWARE to tell Cartridge::Mirror to use the cartridge-header nametable mirroring.
// Integration: Base mapper: defines overridable CPU/PPU address mapping, mirroring and IRQ hooks used by Cartridge and Bus.
MIRROR Mapper::mirror()
{
	return MIRROR::HARDWARE;
}

// TEACHING API: Mapper::irqState
// Default false: a simple mapper does not request CPU interrupts.
// Integration: Base mapper: defines overridable CPU/PPU address mapping, mirroring and IRQ hooks used by Cartridge and Bus.
bool Mapper::irqState()
{
	return false;
}

// TEACHING API: Mapper::irqClear
// Default no-op; interrupt-capable mappers override it to acknowledge an IRQ.
// Integration: Base mapper: defines overridable CPU/PPU address mapping, mirroring and IRQ hooks used by Cartridge and Bus.
void Mapper::irqClear()
{
}

// TEACHING API: Mapper::scanline
// Default no-op; advanced mappers can receive timing signals from the PPU.
// Integration: Base mapper: defines overridable CPU/PPU address mapping, mirroring and IRQ hooks used by Cartridge and Bus.
void Mapper::scanline()
{
}
