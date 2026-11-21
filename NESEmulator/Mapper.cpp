// TEACHING GUIDE — Mapper.cpp
// Default Mapper polymorphic protocol: Cartridge delegates CPU/PPU address translation to 
// mapper subclasses. This base defines default mirroring and optional scanline/IRQ hooks; it 
// does not own PRG/CHR byte storage.
// Read ../READING_ORDER.md for API interactions and upstream code attribution.

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

// Default has no IRQ; MMC3 (Mapper_004) overrides this, and Bus::clock
// forwards active cartridge IRQs to cpu6502::irq().
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
