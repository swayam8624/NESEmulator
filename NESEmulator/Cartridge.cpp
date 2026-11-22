// TEACHING MAP: Cartridge is the boundary between untrusted iNES bytes,
// mapper-controlled PRG/CHR memory, and the emulated CPU/PPU buses.
// The host owns the ROM filename, Cartridge chooses a Mapper_* subclass,
// and Mapper translation determines which PRG/CHR bank a bus address reaches.
//
// INPUT SAFETY: INES1Header.hpp validates magic/format/mapper/mirroring and
// requires exact trainer, PRG and CHR payload lengths. Unsupported NES 2.0
// images are rejected deliberately until extended bank/mapper support exists.
// Every mapper-produced PRG/CHR offset is bounds-checked below. This is a
// safer subset of NES media formats, NOT a claim of full emulator compatibility.
//
// The top-level repository license does not confer redistribution rights
// over commercial ROM data or third-party emulator/pixel-engine source.
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
#include <utility>
#include "INES1Header.hpp"
#include "KairoPixelGameEngine.cppm"
module Kairo.NES;
// TEACHING API: Cartridge::Cartridge
// Read a bounded, explicitly supported iNES1 image into owned PRG/CHR bytes.
// Validate header and complete input before constructing a Mapper object.
// No image becomes valid until a supported mapper is actually created.
// A header with zero CHR ROM banks produces 8 KiB writable CHR RAM, without
// consuming any phantom CHR bytes from disk. The host still checks ImageValid.
Cartridge::Cartridge(const std::string& sFileName)
{
    bImageValid = false;
    std::ifstream input(sFileName, std::ios::binary);
    if (!input) return; // existing API: missing files are invalid, not fatal

    kairo::nes::detail::INES1RomPayload image;
    try
    {
        // Validate signature, format, supported mapper and PRG/CHR sizes
        // before allocating banks, then require a complete optional trainer,
        // PRG ROM and CHR ROM payload. Zero CHR banks means 8 KiB of zeroed
        // CHR RAM; there are no CHR bytes to consume from the ROM file.
        image = kairo::nes::detail::ReadINES1Rom(input);
    }
    catch (const std::invalid_argument&)
    {
        // Malformed/unsupported ROMs retain ImageValid()==false. The host
        // currently tries several cartridge paths and checks this flag.
        // std::bad_alloc or other unexpected host errors still propagate.
        return;
    }

    nMapperID = image.Header.Mapper;
    nPRGBanks = image.Header.PRGBanks;
    nCHRBanks = image.Header.CHRBanks;
    hw_mirror = image.Header.VerticalMirroring ? VERTICAL : HORIZONTAL;
    vPRGMemory = std::move(image.PRG);
    vCHRMemory = std::move(image.CHR);

    // The reader rejects every unsupported mapper BEFORE allocating ROM
    // banks; these cases must stay aligned with its explicit support table.
    switch (nMapperID)
    {
        case  0: pMapper = std::make_shared<Mapper_000>(nPRGBanks, nCHRBanks); break;
        case  1: pMapper = std::make_shared<Mapper_001>(nPRGBanks, nCHRBanks); break;
        case  2: pMapper = std::make_shared<Mapper_002>(nPRGBanks, nCHRBanks); break;
        case  3: pMapper = std::make_shared<Mapper_003>(nPRGBanks, nCHRBanks); break;
        case  4: pMapper = std::make_shared<Mapper_004>(nPRGBanks, nCHRBanks); break;
        case 66: pMapper = std::make_shared<Mapper_066>(nPRGBanks, nCHRBanks); break;
        default: return; // defense in depth if support table later changes
    }
    bImageValid = pMapper != nullptr;
}

// TEACHING API: Cartridge::~Cartridge
// Destroys cartridge-owned buffers and shared mapper when the final owner exits.
// No mapped host pointers should outlive this cartridge.
Cartridge::~Cartridge()
{
}

// TEACHING API: Cartridge::ImageValid
// Reports a fully read supported iNES1 image with a constructed mapper.
// A valid image is not a promise of cycle-accurate mapping or game support.
bool Cartridge::ImageValid()
{
	return bImageValid;
}

// TEACHING API: Cartridge::cpuRead
// For an address sent by Bus::cpuRead, ask the mapper for a PRG bank offset
// or a mapper-supplied value (0xFFFFFFFF sentinel). Reject any invalid
// mapper offset BEFORE indexing the PRG vector (bank registers are mutable).
// On an unmapped address, return false so the Bus can try other devices.
bool Cartridge::cpuRead(uint16_t addr, uint8_t &data)
{
    if (!pMapper) return false; // invalid or unsupported cartridge
	uint32_t mapped_addr = 0;
	if (pMapper->cpuMapRead(addr, mapped_addr, data))
	{
		if (mapped_addr == 0xFFFFFFFF)
		{
			// Mapper has actually set the data value, for example cartridge based RAM
			return true;
		}
		else
		{
			// Mapper has produced an offset into cartridge bank memory
			if (mapped_addr >= vPRGMemory.size()) return false;
            data = vPRGMemory[mapped_addr];
		}
		return true;
	}
	else
		return false;
}

// TEACHING API: Cartridge::cpuWrite
// For a Bus write, delegate mapper register/bank handling before optional
// PRG-memory access. Some writes update mapper state rather than ROM bytes.
// An out-of-range mapper result cannot write past the PRG storage vector.
bool Cartridge::cpuWrite(uint16_t addr, uint8_t data)
{
    if (!pMapper) return false;
	uint32_t mapped_addr = 0;
	if (pMapper->cpuMapWrite(addr, mapped_addr, data))
	{
		if (mapped_addr == 0xFFFFFFFF)
		{
			// Mapper has actually set the data value, for example cartridge based RAM
			return true;
		}
		else
		{
			// Mapper has produced an offset into cartridge bank memory
			if (mapped_addr >= vPRGMemory.size()) return false;
            vPRGMemory[mapped_addr] = data;
		}
		return true;
	}
	else
		return false;
}

// TEACHING API: Cartridge::ppuRead
// Read CHR bytes for pattern rendering using an offset supplied by the
// active mapper. The caller is ppu2C02::ppuRead, not the host display.
bool Cartridge::ppuRead(uint16_t addr, uint8_t & data)
{
    if (!pMapper) return false;
	uint32_t mapped_addr = 0;
	if (pMapper->ppuMapRead(addr, mapped_addr))
	{
		if (mapped_addr >= vCHRMemory.size()) return false;
        data = vCHRMemory[mapped_addr];
		return true;
	}
	else
		return false;
}

// TEACHING API: Cartridge::ppuWrite
// Accept CHR writes when the mapper permits them (for example CHR RAM).
// The PPU drives this operation; bank mapping is mapper-controlled.
bool Cartridge::ppuWrite(uint16_t addr, uint8_t data)
{
    if (!pMapper) return false;
	uint32_t mapped_addr = 0;
	if (pMapper->ppuMapWrite(addr, mapped_addr))
	{
		if (mapped_addr >= vCHRMemory.size()) return false;
        vCHRMemory[mapped_addr] = data;
		return true;
	}
	else
		return false;
}


// TEACHING API: Cartridge::reset
// Reset mapper registers/bank selection without re-reading or destroying the
// ROM buffers. Called by Bus::reset.
void Cartridge::reset()
{
	// Note: This does not reset the ROM contents,
	// but does reset the mapper.
	if (pMapper != nullptr)
		pMapper->reset();
}

// TEACHING API: Cartridge::Mirror
// Resolve nametable mirroring: mapper override or hardware-mirroring value
// from the ROM header. ppu2C02 uses this to map $2000-$2FFF tables.
MIRROR Cartridge::Mirror()
{
    if (!pMapper) return hw_mirror; // invalid image, no mapper register
	MIRROR m = pMapper->mirror();
	if (m == MIRROR::HARDWARE)
	{
		// Mirror configuration was defined
		// in hardware via soldering
		return hw_mirror;
	}
	else
	{
		// Mirror configuration can be
		// dynamically set via mapper
		return m;
	}
}

// TEACHING API: Cartridge::GetMapper
// Expose the active mapper so the bus can inspect IRQ state and related
// hardware signals. Callers must account for a null mapper on invalid ROMs.
std::shared_ptr<Mapper> Cartridge::GetMapper()
{
	return pMapper;
}
