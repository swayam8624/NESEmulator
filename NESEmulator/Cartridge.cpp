// TEACHING MAP: Cartridge is the boundary between untrusted iNES bytes,
// mapper-controlled PRG/CHR memory, and the emulated CPU/PPU buses.
// The host owns the ROM filename, Cartridge chooses a Mapper_* subclass,
// and Mapper translation determines which PRG/CHR bank a bus address reaches.
//
// WARNING FROM SOURCE REVIEW: header and payload read results are not
// consistently checked, and the mapper switch has no explicit unsupported
// case before setting bImageValid=true. In particular, accepting a header
// does not guarantee pMapper is non-null; later bus IRQ access dereferences
// GetMapper(). This must be repaired and tested separately from teaching
// comments before accepting arbitrary user-supplied ROMs as safe.
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
#include "KairoPixelGameEngine.cppm"
module Kairo.NES;
// Read the ROM header, select a supported mapper and allocate PRG/CHR. Header read lengths and
// allocation bounds are correctness and security preconditions.
// TEACHING API: Cartridge::Cartridge
// Loads and interprets ROM metadata, allocates PRG/CHR storage, and chooses
// a Mapper_* implementation. Bus and PPU later read only through this object.
// IMPORTANT: current code does not consistently validate short reads, header
// signature or unsupported mapper IDs; a valid flag is not yet a safety proof.
// Parse iNES header, optional trainer, PRG/CHR banks and mapper choice.
// Read failures, truncated buffers and unknown mapper IDs must reject
// the cartridge; current implementation has incomplete validation.
Cartridge::Cartridge(const std::string& sFileName)
{
	// iNES Format Header
	struct sHeader
	{
		char name[4];
		uint8_t prg_rom_chunks;
		uint8_t chr_rom_chunks;
		uint8_t mapper1;
		uint8_t mapper2;
		uint8_t prg_ram_size;
		uint8_t tv_system1;
		uint8_t tv_system2;
		char unused[5];
	} header;

	bImageValid = false;

	std::ifstream ifs;
	ifs.open(sFileName, std::ifstream::binary);
	if (ifs.is_open())
	{
		// Read file header
		ifs.read((char*)&header, sizeof(sHeader));

		// If a "trainer" exists we just need to read past
		// it before we get to the good stuff
		if (header.mapper1 & 0x04)
			ifs.seekg(512, std::ios_base::cur);

		// Determine Mapper ID
		nMapperID = ((header.mapper2 >> 4) << 4) | (header.mapper1 >> 4);
		hw_mirror = (header.mapper1 & 0x01) ? VERTICAL : HORIZONTAL;

		// "Discover" File Format
		uint8_t nFileType = 1;
		if ((header.mapper2 & 0x0C) == 0x08) nFileType = 2;

		if (nFileType == 0)
		{

		}

		if (nFileType == 1)
		{
			nPRGBanks = header.prg_rom_chunks;
			vPRGMemory.resize(nPRGBanks * 16384);
			ifs.read((char*)vPRGMemory.data(), vPRGMemory.size());

			nCHRBanks = header.chr_rom_chunks;
			if (nCHRBanks == 0)
			{
				// Create CHR RAM
				vCHRMemory.resize(8192);
			}
			else
			{
				// Allocate for ROM
				vCHRMemory.resize(nCHRBanks * 8192);
			}
			ifs.read((char*)vCHRMemory.data(), vCHRMemory.size());
		}

		if (nFileType == 2)
		{
			nPRGBanks = ((header.prg_ram_size & 0x07) << 8) | header.prg_rom_chunks;
			vPRGMemory.resize(nPRGBanks * 16384);
			ifs.read((char*)vPRGMemory.data(), vPRGMemory.size());

			nCHRBanks = ((header.prg_ram_size & 0x38) << 8) | header.chr_rom_chunks;
			vCHRMemory.resize(nCHRBanks * 8192);
			ifs.read((char*)vCHRMemory.data(), vCHRMemory.size());
		}

		// Load appropriate mapper
		switch (nMapperID)
		{
		case   0: pMapper = std::make_shared<Mapper_000>(nPRGBanks, nCHRBanks); break;
		case   1: pMapper = std::make_shared<Mapper_001>(nPRGBanks, nCHRBanks); break;
		case   2: pMapper = std::make_shared<Mapper_002>(nPRGBanks, nCHRBanks); break;
		case   3: pMapper = std::make_shared<Mapper_003>(nPRGBanks, nCHRBanks); break;
		case   4: pMapper = std::make_shared<Mapper_004>(nPRGBanks, nCHRBanks); break;
		case  66: pMapper = std::make_shared<Mapper_066>(nPRGBanks, nCHRBanks); break;

		}

		bImageValid = true;
		ifs.close();
	}

}


// TEACHING API: Cartridge::~Cartridge
// Destroys cartridge-owned buffers and shared mapper when the final owner exits.
// No mapped host pointers should outlive this cartridge.
Cartridge::~Cartridge()
{
}

// TEACHING API: Cartridge::ImageValid
// Reports the loader's current validity flag to main.cpp before cartridge
// insertion. Present implementation can mark unsupported mapper IDs valid;
// documented behavior must not be mistaken for safe input validation.
bool Cartridge::ImageValid()
{
	return bImageValid;
}

// TEACHING API: Cartridge::cpuRead
// For an address sent by Bus::cpuRead, ask the mapper for a PRG bank offset
// or a mapper-supplied value (0xFFFFFFFF sentinel). On an unmapped address,
// return false so the Bus can try other devices.
bool Cartridge::cpuRead(uint16_t addr, uint8_t &data)
{
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
bool Cartridge::cpuWrite(uint16_t addr, uint8_t data)
{
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
	uint32_t mapped_addr = 0;
	if (pMapper->ppuMapRead(addr, mapped_addr))
	{
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
	uint32_t mapped_addr = 0;
	if (pMapper->ppuMapWrite(addr, mapped_addr))
	{
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
