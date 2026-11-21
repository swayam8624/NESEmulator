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
Mapper::Mapper(uint8_t prgBanks, uint8_t chrBanks)
{
	nPRGBanks = prgBanks;
	nCHRBanks = chrBanks;
}


Mapper::~Mapper()
{
}

void Mapper::reset()
{

}

MIRROR Mapper::mirror()
{
	return MIRROR::HARDWARE;
}

bool Mapper::irqState()
{
	return false;
}

void Mapper::irqClear()
{
}

void Mapper::scanline()
{
}
