// TEACHING FILE GUIDE
// Mapper 4/MMC3 combines CPU-visible PRG bank switching, PPU-visible
// CHR bank switching, nametable mirroring and scanline-driven IRQ state.
// Cartridge delegates address translation here; Bus receives irqState
// and signals the 6502. PPU scanline/A12 timing influences IRQ precision.
//
// This source increments the IRQ state from a scanline notification;
// it is not a cycle-accurate MMC3 PPU A12 edge detector. Games that rely
// on precise mid-scanline bank changes/IRQ behavior need independent tests.
// MMC3-style bank switching and scanline IRQ behavior. CPU register writes
// configure PRG/CHR windows; PPU rendering/scanline progress can trigger
// mapper interrupt state observed by the CPU through Bus.
// This mapper has more timing-sensitive behavior than NROM/UxROM; validate
// A12/scanline semantics and mirroring instead of just loading one game.
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
// TEACHING API: Mapper_004::Mapper_004
// Allocate mapper-controlled cartridge RAM. PRG and CHR mapping tables are populated from register state.
// Integration: MMC3: independent PRG/CHR bank windows, mirroring register and scanline-based IRQ state.
Mapper_004::Mapper_004(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks)
{
	vRAMStatic.resize(32 * 1024);
}


// TEACHING API: Mapper_004::~Mapper_004
// Cartridge currently stores make_shared<Derived> objects as shared_ptr<Mapper>;
// the shared control block retains the concrete deleter. Mapper's base
// destructor is not virtual, so deleting a derived object through a raw
// Mapper* would be unsafe. Mapper instances do not own PRG/CHR ROM bytes.
Mapper_004::~Mapper_004()
{
}

// TEACHING API: Mapper_004::cpuMapRead
// Handle cartridge RAM or resolve one of four 8 KiB PRG windows using the current bank table.
// Integration: MMC3: independent PRG/CHR bank windows, mirroring register and scanline-based IRQ state.
// MMC3 PRG WINDOWS: $8000/$A000/$C000/$E000 each address an 8 KiB
// region; pPRGBank[0..3] chooses which cartridge PRG bank supplies bytes.
// Cartridge::cpuRead must verify this computed offset against PRG storage.
// The $6000-$7FFF special sentinel 0xFFFFFFFF means mapper-owned RAM
// already supplied data; it is not a valid vector index into PRG ROM.
bool Mapper_004::cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data)
{
	if (addr >= 0x6000 && addr <= 0x7FFF)
	{
		// Write is to static ram on cartridge
		mapped_addr = 0xFFFFFFFF;

		// Write data to RAM
		data = vRAMStatic[addr & 0x1FFF];

		// Signal mapper has handled request
		return true;
	}


	if (addr >= 0x8000 && addr <= 0x9FFF)
	{
		mapped_addr = pPRGBank[0] + (addr & 0x1FFF);
		return true;
	}

	if (addr >= 0xA000 && addr <= 0xBFFF)
	{
		mapped_addr = pPRGBank[1] + (addr & 0x1FFF);
		return true;
	}

	if (addr >= 0xC000 && addr <= 0xDFFF)
	{
		mapped_addr = pPRGBank[2] + (addr & 0x1FFF);
		return true;
	}

	if (addr >= 0xE000 && addr <= 0xFFFF)
	{
		mapped_addr = pPRGBank[3] + (addr & 0x1FFF);
		return true;
	}

	return false;
}

// TEACHING API: Mapper_004::cpuMapWrite
// Decode bank-select/data, mirroring, RAM protection and IRQ registers. This function updates pointers used by future CPU/PPU reads; the PRG-RAM-protect register remains TODO.
// Integration: MMC3: independent PRG/CHR bank windows, mirroring register and scanline-based IRQ state.
// THIS WRITE IS A REGISTER TRANSACTION, not always a PRG ROM write:
// MMC3 control/data register pairs select bank mode, CHR inversion,
// mirroring and IRQ reload/enable policy. Cartridge recognizes the
// 0xFFFFFFFF sentinel so mapper-handled writes do not overwrite PRG ROM.
// Cross-check each indexed bank against the source ROM's real bank count.
bool Mapper_004::cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data)
{
	if (addr >= 0x6000 && addr <= 0x7FFF)
	{
		// Write is to static ram on cartridge
		mapped_addr = 0xFFFFFFFF;

		// Write data to RAM
		vRAMStatic[addr & 0x1FFF] = data;

		// Signal mapper has handled request
		return true;
	}

	if (addr >= 0x8000 && addr <= 0x9FFF)
	{
		// Bank Select
		if (!(addr & 0x0001))
		{
			nTargetRegister = data & 0x07;
			bPRGBankMode = (data & 0x40);
			bCHRInversion = (data & 0x80);
		}
		else
		{
			// Update target register
			pRegister[nTargetRegister] = data;

			// Update Pointer Table
			if (bCHRInversion)
			{
				pCHRBank[0] = pRegister[2] * 0x0400;
				pCHRBank[1] = pRegister[3] * 0x0400;
				pCHRBank[2] = pRegister[4] * 0x0400;
				pCHRBank[3] = pRegister[5] * 0x0400;
				pCHRBank[4] = (pRegister[0] & 0xFE) * 0x0400;
				pCHRBank[5] = pRegister[0] * 0x0400 + 0x0400;
				pCHRBank[6] = (pRegister[1] & 0xFE) * 0x0400;
				pCHRBank[7] = pRegister[1] * 0x0400 + 0x0400;
			}
			else
			{
				pCHRBank[0] = (pRegister[0] & 0xFE) * 0x0400;
				pCHRBank[1] = pRegister[0] * 0x0400 + 0x0400;
				pCHRBank[2] = (pRegister[1] & 0xFE) * 0x0400;
				pCHRBank[3] = pRegister[1] * 0x0400 + 0x0400;
				pCHRBank[4] = pRegister[2] * 0x0400;
				pCHRBank[5] = pRegister[3] * 0x0400;
				pCHRBank[6] = pRegister[4] * 0x0400;
				pCHRBank[7] = pRegister[5] * 0x0400;
			}

			if (bPRGBankMode)
			{
				pPRGBank[2] = (pRegister[6] & 0x3F) * 0x2000;
				pPRGBank[0] = (nPRGBanks * 2 - 2) * 0x2000;
			}
			else
			{
				pPRGBank[0] = (pRegister[6] & 0x3F) * 0x2000;
				pPRGBank[2] = (nPRGBanks * 2 - 2) * 0x2000;
			}

			pPRGBank[1] = (pRegister[7] & 0x3F) * 0x2000;
			pPRGBank[3] = (nPRGBanks * 2 - 1) * 0x2000;

		}

		return false;
	}

	if (addr >= 0xA000 && addr <= 0xBFFF)
	{
		if (!(addr & 0x0001))
		{
			// Mirroring
			if (data & 0x01)
				mirrormode = MIRROR::HORIZONTAL;
			else
				mirrormode = MIRROR::VERTICAL;
		}
		else
		{
			// PRG Ram Protect
			// TODO:
		}
		return false;
	}

	if (addr >= 0xC000 && addr <= 0xDFFF)
	{
		if (!(addr & 0x0001))
		{
			nIRQReload = data;
		}
		else
		{
			nIRQCounter = 0x0000;
		}
		return false;
	}

	if (addr >= 0xE000 && addr <= 0xFFFF)
	{
		if (!(addr & 0x0001))
		{
			bIRQEnable = false;
			bIRQActive = false;
		}
		else
		{
			bIRQEnable = true;
		}
		return false;
	}



	return false;
}

// TEACHING API: Mapper_004::ppuMapRead
// Map a 1 KiB CHR address window via the active CHR bank table, including inversion mode.
// Integration: MMC3: independent PRG/CHR bank windows, mirroring register and scanline-based IRQ state.
// PPU CHR WINDOWS: map the visible $0000-$1FFF pattern address through
// the eight computed 1 KiB window offsets. CHR inversion swaps the
// arrangement chosen by MMC3 bank-control registers. The Cartridge then
// validates the returned byte offset against its owned CHR vector.
bool Mapper_004::ppuMapRead(uint16_t addr, uint32_t &mapped_addr)
{
	if (addr >= 0x0000 && addr <= 0x03FF)
	{
		mapped_addr = pCHRBank[0] + (addr & 0x03FF);
		return true;
	}

	if (addr >= 0x0400 && addr <= 0x07FF)
	{
		mapped_addr = pCHRBank[1] + (addr & 0x03FF);
		return true;
	}

	if (addr >= 0x0800 && addr <= 0x0BFF)
	{
		mapped_addr = pCHRBank[2] + (addr & 0x03FF);
		return true;
	}

	if (addr >= 0x0C00 && addr <= 0x0FFF)
	{
		mapped_addr = pCHRBank[3] + (addr & 0x03FF);
		return true;
	}

	if (addr >= 0x1000 && addr <= 0x13FF)
	{
		mapped_addr = pCHRBank[4] + (addr & 0x03FF);
		return true;
	}

	if (addr >= 0x1400 && addr <= 0x17FF)
	{
		mapped_addr = pCHRBank[5] + (addr & 0x03FF);
		return true;
	}

	if (addr >= 0x1800 && addr <= 0x1BFF)
	{
		mapped_addr = pCHRBank[6] + (addr & 0x03FF);
		return true;
	}

	if (addr >= 0x1C00 && addr <= 0x1FFF)
	{
		mapped_addr = pCHRBank[7] + (addr & 0x03FF);
		return true;
	}

	return false;
}

// TEACHING API: Mapper_004::ppuMapWrite
// The current implementation does not provide a writable CHR path; validate CHR RAM compatibility.
// Integration: MMC3: independent PRG/CHR bank windows, mirroring register and scanline-based IRQ state.
bool Mapper_004::ppuMapWrite(uint16_t addr, uint32_t &mapped_addr)
{
	
	
	return false;
}

// TEACHING API: Mapper_004::reset
// Initialize bank table/registers and IRQ state to the implemented mapper's reset policy.
// Integration: MMC3: independent PRG/CHR bank windows, mirroring register and scanline-based IRQ state.
// Reset control registers and initial PRG/CHR window offsets after a
// loaded cartridge is installed. This method does NOT zero ROM arrays;
// PRG/CHR bytes remain owned by Cartridge. Test small PRG bank counts
// carefully because the last-two-bank arithmetic assumes valid capacity.
void Mapper_004::reset()
{
	nTargetRegister = 0x00;
	bPRGBankMode = false;
	bCHRInversion = false;
	mirrormode = MIRROR::HORIZONTAL;

	bIRQActive = false;
	bIRQEnable = false;
	bIRQUpdate = false;
	nIRQCounter = 0x0000;
	nIRQReload = 0x0000;

	for (int i = 0; i < 4; i++)	pPRGBank[i] = 0;
	for (int i = 0; i < 8; i++) { pCHRBank[i] = 0; pRegister[i] = 0; }

	pPRGBank[0] = 0 * 0x2000;
	pPRGBank[1] = 1 * 0x2000;
	pPRGBank[2] = (nPRGBanks * 2 - 2) * 0x2000;
	pPRGBank[3] = (nPRGBanks * 2 - 1) * 0x2000;
}


// TEACHING API: Mapper_004::irqState
// Expose whether the scanline IRQ latch is active so Bus can signal the CPU.
// Integration: MMC3: independent PRG/CHR bank windows, mirroring register and scanline-based IRQ state.
// The Bus polls this latch after PPU/CPU scheduling; it is not a
// direct CPU clock interrupt. IRQ state is independent of whether the
// CPU is currently masking interrupts with its status I bit.
bool Mapper_004::irqState()
{
	return bIRQActive;
}

// TEACHING API: Mapper_004::irqClear
// Clear the latched interrupt request after the CPU/bus handles it.
// Integration: MMC3: independent PRG/CHR bank windows, mirroring register and scanline-based IRQ state.
void Mapper_004::irqClear()
{
	bIRQActive = false;
}

// TEACHING API: Mapper_004::scanline
// Update the IRQ counter on the PPU's scanline notification. Timing differs from cycle/A12-edge-accurate hardware and needs dedicated tests.
// Integration: MMC3: independent PRG/CHR bank windows, mirroring register and scanline-based IRQ state.
// Update the mapper IRQ latch/counter from the PPU's scanline
// callback. This simplified interface is less precise than a true A12
// rising-edge-qualified MMC3 counter; document any accuracy tests.
// WORKED IRQ COUNTER: if reload=2, enabled=true, initial counter=0,
// successive calls set counter=2, then 1, then 0 and assert IRQ.
// This code counts explicit PPU scanline notifications, not hardware's
// qualified PPU A12 rises; therefore its timing is an approximation.
void Mapper_004::scanline()
{
	if (nIRQCounter == 0)
	{		
		nIRQCounter = nIRQReload;
	}
	else
		nIRQCounter--;

	if (nIRQCounter == 0 && bIRQEnable)
	{
		bIRQActive = true;
	}
	
}

// TEACHING API: Mapper_004::mirror
// Return MMC3 mirroring register state so PPU nametable reads/writes resolve correctly.
// Integration: MMC3: independent PRG/CHR bank windows, mirroring register and scanline-based IRQ state.
MIRROR Mapper_004::mirror()
{
	return mirrormode;
}
