// SOURCE STUDY NOTE:
// Base mapper extension points. Cartridge.cpp owns a Mapper instance selected from the ROM header.
// cpuMap*/ppuMap* either translate addresses into PRG/CHR offsets or decline the address; derived
// Mapper_*.cpp files implement specific cartridge hardware.
// Guided cross-file links: docs/SOURCE_READING_ORDER.md.

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
