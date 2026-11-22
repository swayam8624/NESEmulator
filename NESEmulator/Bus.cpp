// TEACHING MAP: Bus owns the emulated machine interconnect, not the
// application window. The CPU calls cpuRead/cpuWrite for mapped devices;
// Cartridge delegates PRG/CHR bank translation to the active Mapper;
// PPU renders pixels and asserts NMI, while APU generates audio samples.
//
// CPU RAM is 2 KiB mirrored through $0000-$1FFF (mask $07FF).
// Eight PPU registers mirror through $2000-$3FFF (mask $0007).
// $4014 initiates a 256-byte CPU-memory-to-PPU-OAM DMA, stalling the CPU;
// controller state is latched into serial shift registers at $4016/$4017.
//
// Every Bus::clock call advances one PPU tick; CPU ticks occur every
// third bus tick unless DMA is active. NMI/mapper IRQ are then delivered,
// and the host receives an audio-ready flag when its sample period elapses.
// Cycle accounting, interrupt timing and register side effects determine
// game compatibility, not only how often a visible frame is drawn.
// See cpu6502.cpp, ppu2C02.cpp, Mapper_004.cpp and docs/SOURCE_READING_ORDER.md.
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
// TEACHING API: Bus::Bus
// Creates the console interconnect and gives the CPU a pointer to this bus.
// The CPU then calls Bus::cpuRead/cpuWrite for all memory-mapped operations.
Bus::Bus()
{
	// Connect CPU to communication bus
	cpu.ConnectBus(this);
	
}


// TEACHING API: Bus::~Bus
// Releases the bus object; device members and shared cartridge ownership follow
// their own C++ lifetimes. This is not a reset of the emulated console.
Bus::~Bus()
{
}

// TEACHING FUNCTION: Convert output audio sample rate to a time interval and establish the NES master-clock interval. The host may request 44.1 kHz, but emulation timing remains based on console cycles.
// TEACHING API: Bus::SetSampleFrequency
// Converts a host output rate into seconds per audio sample. The APU advances
// on emulated ticks, while the host decides how often a sample is consumed.
// API: sample_rate is in Hz; a zero rate is currently not rejected.
void Bus::SetSampleFrequency(uint32_t sample_rate)
{
	dAudioTimePerSystemSample = 1.0 / (double)sample_rate;
	dAudioTimePerNESClock = 1.0 / 5369318.0; // PPU Clock Frequency
}

// CPU write decoder: cartridge receives the address first; later branches select mirrored RAM,
// PPU/APU, DMA or controller hardware. Register writes have side effects, unlike ordinary RAM
// writes.
// TEACHING API: Bus::cpuWrite
// Decodes CPU-visible address ranges: cartridge first, then mirrored RAM, PPU
// registers, APU, DMA and controller latch. Writes can trigger hardware effects.
// Consumers: 6502 instructions and host debugging; collaborators: Cartridge/PPU/APU.
// A write can mutate mapper bank/IRQ state, PPU registers, APU channels,
// DMA scheduling or the controller latch. Callers must not assume this
// is simply an array assignment.
// Bus writes route through the cartridge mapper first, then RAM/PPU/
// APU/controller/DMA registers. CPU address $0000-$1FFF mirrors only
// 2 KiB RAM; $2000-$3FFF mirrors 8 PPU registers. $4014 schedules
// 256 OAM bytes rather than writing a normal RAM page.
void Bus::cpuWrite(uint16_t addr, uint8_t data)
{	
	if (cart->cpuWrite(addr, data))
	{
		// The cartridge "sees all" and has the facility to veto
		// the propagation of the bus transaction if it requires.
		// This allows the cartridge to map any address to some
		// other data, including the facility to divert transactions
		// with other physical devices. The NES does not do this
		// but I figured it might be quite a flexible way of adding
		// "custom" hardware to the NES in the future!
	}
	else if (addr >= 0x0000 && addr <= 0x1FFF)
	{
		// System RAM Address Range. The range covers 8KB, though
		// there is only 2KB available. That 2KB is "mirrored"
		// through this address range. Using bitwise AND to mask
		// the bottom 11 bits is the same as addr % 2048.
		cpuRam[addr & 0x07FF] = data;

	}
	else if (addr >= 0x2000 && addr <= 0x3FFF)
	{
		// PPU Address range. The PPU only has 8 primary registers
		// and these are repeated throughout this range. We can
		// use bitwise AND operation to mask the bottom 3 bits, 
		// which is the equivalent of addr % 8.
		ppu.cpuWrite(addr & 0x0007, data);
	}	
	else if ((addr >= 0x4000 && addr <= 0x4013) || addr == 0x4015 || addr == 0x4017) //  NES APU
	{
		apu.cpuWrite(addr, data);
	}
	else if (addr == 0x4014)
	{
		// A write to this address initiates a DMA transfer
		dma_page = data;
		dma_addr = 0x00;
		dma_transfer = true;						
	}
	else if (addr >= 0x4016 && addr <= 0x4017)
	{
		// "Lock In" controller state at this time
		controller_state[addr & 0x0001] = controller[addr & 0x0001];
	}
	
}

// TEACHING API: Bus::cpuRead
// Decodes CPU-visible reads, including controller shift-state and PPU status.
// bReadOnly requests inspection that should not mutate device state; the PPU
// receives the flag. The CPU and disassembler call this interface.
// PPU inspection receives bReadOnly, but the controller shift branch
// below shifts regardless of this flag. A debugger that uses read-only
// bus access at $4016/$4017 may still consume input bits: add a
// regression test before claiming side-effect-free inspection.
// Reads are device *operations*: PPU status may clear vblank,
// controller shift registers consume serial bits, and cartridge
// mapper may translate a PRG bank. Debug bReadOnly is not fully
// respected by controller reads in this implementation, so a
// debugger may change input state merely by inspecting $4016.
uint8_t Bus::cpuRead(uint16_t addr, bool bReadOnly)
{
	uint8_t data = 0x00;	
	if (cart->cpuRead(addr, data))
	{
		// Cartridge Address Range
	}
	else if (addr >= 0x0000 && addr <= 0x1FFF)
	{
		// System RAM Address Range, mirrored every 2048
		data = cpuRam[addr & 0x07FF];
	}
	else if (addr >= 0x2000 && addr <= 0x3FFF)
	{
		// PPU Address range, mirrored every 8
		data = ppu.cpuRead(addr & 0x0007, bReadOnly);
	}
	else if (addr == 0x4015)
	{
		// APU Read Status
		data = apu.cpuRead(addr);
	}
	else if (addr >= 0x4016 && addr <= 0x4017)
	{
		// Read out the MSB of the controller status word
		data = (controller_state[addr & 0x0001] & 0x80) > 0;
		controller_state[addr & 0x0001] <<= 1;
	}

	return data;
}

// TEACHING FUNCTION: Connect loaded ROM state to bus/PPU. Cartridge owns PRG/CHR and mapper logic; this operation must precede meaningful instruction execution.
// TEACHING API: Bus::insertCartridge
// Wires an already-created cartridge into the machine and PPU so both CPU
// program reads and PPU pattern fetches use the same mapper/bank configuration.
// Call before reset/clock; ownership uses std::shared_ptr.
// Attach the same mapper-backed Cartridge to the CPU bus and PPU
// so PRG (CPU), CHR (PPU) and mapper IRQ state stay consistent.
 // A non-null and supported mapper is required before clocking.
void Bus::insertCartridge(const std::shared_ptr<Cartridge>& cartridge)
{
	// Connects cartridge to both Main Bus and CPU Bus
	this->cart = cartridge;
	ppu.ConnectCartridge(cartridge);
}

// TEACHING FUNCTION: Restore CPU/PPU/APU/mapper timing state for a fresh machine run. A reset is not a reload of ROM bytes; compare Cartridge::reset.
// TEACHING API: Bus::reset
// Resets emulated devices, counters and DMA state; ROM contents remain owned
// by Cartridge. See CPU reset-vector reads and PPU reset state for consequences.
// Reinitialize CPU/APU/PPU clocked state for a new emulation run.
 // Resetting devices is not equivalent to constructing a different
 // cartridge/mapper; input and DMA transient state also matter.
void Bus::reset()
{
	cart->reset();
	cpu.reset();
	ppu.reset();
	nSystemClockCounter = 0;
	dma_page = 0x00;
	dma_addr = 0x00;
	dma_data = 0x00;
	dma_dummy = true;
	dma_transfer = false;
}

// TEACHING FUNCTION: Advance console devices at their relative clock rates; handle OAM DMA stalls and coordinate audio sampling. main.cpp uses this to advance emulated time without directly clocking devices.
// TEACHING API: Bus::clock
// Advances PPU/APU per system tick, CPU every third tick except during OAM
// DMA, and handles NMI/mapper IRQ. It can signal when a host audio sample is
// ready. Main's update loop repeatedly calls this function to advance time.
// The host determines real-time pacing, but this routine determines
// emulated hardware order. DMA alternates CPU reads and PPU OAM writes
// on eligible clock slots; PPU NMI and mapper IRQ reach CPU afterwards.
// One *master PPU timing tick*. PPU and APU clock every call; CPU
// consumes a slot every third tick unless OAM DMA steals it.
 // DMA alternates bus read and PPU OAM write across slots, with a
 // dummy alignment phase. After clocks, audio sample readiness is
 // calculated and PPU NMI / mapper IRQ requests reach the CPU.
 // This is scheduling semantics, not audio-device playback.
bool Bus::clock()
{
	// Clocking. The heart and soul of an emulator. The running
	// frequency is controlled by whatever calls this function.
	// So here we "divide" the clock as necessary and call
	// the peripheral devices clock() function at the correct
	// times.

	// The fastest clock frequency the digital system cares
	// about is equivalent to the PPU clock. So the PPU is clocked
	// each time this function is called...
	ppu.clock();

	// ...also clock the APU
	apu.clock();

	// The CPU runs 3 times slower than the PPU so we only call its
	// clock() function every 3 times this function is called. We
	// have a global counter to keep track of this.
    // NTSC-style 3:1 PPU-to-CPU scheduling is represented as one
    // CPU slot per three Bus::clock calls. Mid-instruction CPU bus
    // micro-operations are not modeled cycle-by-cycle by cpu6502.
	if (nSystemClockCounter % 3 == 0)
	{
		// Is the system performing a DMA transfer form CPU memory to 
		// OAM memory on PPU?...
		if (dma_transfer)
		{
			// ...Yes! We need to wait until the next even CPU clock cycle
			// before it starts...
			if (dma_dummy)
			{
				// ...So hang around in here each clock until 1 or 2 cycles
				// have elapsed...
				if (nSystemClockCounter % 2 == 1)
				{
					// ...and finally allow DMA to start
					dma_dummy = false;
				}
			}
			else
			{
				// DMA can take place!
				if (nSystemClockCounter % 2 == 0)
				{
					// On even clock cycles, read from CPU bus
					dma_data = cpuRead(dma_page << 8 | dma_addr);
				}
				else
				{
					// On odd clock cycles, write to PPU OAM
					ppu.pOAM[dma_addr] = dma_data;
					// Increment the lo byte of the address
					dma_addr++;
					// If this wraps around, we know that 256
					// bytes have been written, so end the DMA
					// transfer, and proceed as normal
					if (dma_addr == 0x00)
					{
						dma_transfer = false;
						dma_dummy = true;
					}
				}
			}
		}
		else
		{
			// No DMA happening, the CPU is in control of its
			// own destiny. Go forth my friend and calculate
			// awesomeness for many generations to come...
			cpu.clock();
		}		
	}

	// Synchronising with Audio
	bool bAudioSampleReady = false;
	dAudioTime += dAudioTimePerNESClock;
	if (dAudioTime >= dAudioTimePerSystemSample)
	{
		dAudioTime -= dAudioTimePerSystemSample;
		dAudioSample = apu.GetOutputSample();
		bAudioSampleReady = true;
	}

	// The PPU is capable of emitting an interrupt to indicate the
	// vertical blanking period has been entered. If it has, we need
	// to send that irq to the CPU.
	if (ppu.nmi)
	{
		ppu.nmi = false;
		cpu.nmi();
	}

	
	// Check if cartridge is requesting IRQ
    // The mapper may assert a cartridge IRQ after PPU timing.
    // Cartridge loading must reject unsupported IDs so GetMapper
    // is never null; current loader validation needs hardening.
	if (cart->GetMapper()->irqState())
	{
		cart->GetMapper()->irqClear();
		cpu.irq();		
	}

	nSystemClockCounter++;

	return bAudioSampleReady;
}
