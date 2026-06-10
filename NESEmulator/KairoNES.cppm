module;
#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "KairoPixelGameEngine.cppm"

export module Kairo.NES;

export {
enum MIRROR
{
	HARDWARE,
	HORIZONTAL,
	VERTICAL,
	ONESCREEN_LO,
	ONESCREEN_HI,
};

class Mapper
{
public:
	Mapper(uint8_t prgBanks, uint8_t chrBanks);
	~Mapper();

public:
	// Transform CPU bus address into PRG ROM offset
	virtual bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data)	 = 0;
	virtual bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0)	 = 0;
	
	// Transform PPU bus address into CHR ROM offset
	virtual bool ppuMapRead(uint16_t addr, uint32_t &mapped_addr)	 = 0;
	virtual bool ppuMapWrite(uint16_t addr, uint32_t &mapped_addr)	 = 0;

	// Reset mapper to known state
	virtual void reset() = 0;

	// Get Mirror mode if mapper is in control
	virtual MIRROR mirror();

	// IRQ Interface
	virtual bool irqState();
	virtual void irqClear();

	// Scanline Counting
	virtual void scanline();

protected:
	// These are stored locally as many of the mappers require this information
	uint8_t nPRGBanks = 0;
	uint8_t nCHRBanks = 0;
};


class Mapper_000 : public Mapper
{
public:
	Mapper_000(uint8_t prgBanks, uint8_t chrBanks);
	~Mapper_000();

public:
	bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) override;
	bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) override;
	bool ppuMapRead(uint16_t addr, uint32_t &mapped_addr) override;
	bool ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) override;
	void reset() override;

	// No local equipment required
};


class Mapper_001 : public Mapper
{
public:
	Mapper_001(uint8_t prgBanks, uint8_t chrBanks);
	~Mapper_001();

	bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) override;
	bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) override;
	bool ppuMapRead(uint16_t addr, uint32_t &mapped_addr) override;
	bool ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) override;
	void reset() override;
	MIRROR mirror() override;

private:
	uint8_t nCHRBankSelect4Lo = 0x00;
	uint8_t nCHRBankSelect4Hi = 0x00;
	uint8_t nCHRBankSelect8 = 0x00;

	uint8_t nPRGBankSelect16Lo = 0x00;
	uint8_t nPRGBankSelect16Hi = 0x00;
	uint8_t nPRGBankSelect32 = 0x00;

	uint8_t nLoadRegister = 0x00;
	uint8_t nLoadRegisterCount = 0x00;
	uint8_t nControlRegister = 0x00;

	MIRROR mirrormode = MIRROR::HORIZONTAL;

	std::vector<uint8_t> vRAMStatic;
};


class Mapper_002 : public Mapper
{
public:
	Mapper_002(uint8_t prgBanks, uint8_t chrBanks);
	~Mapper_002();

	bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) override;
	bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) override;
	bool ppuMapRead(uint16_t addr, uint32_t &mapped_addr) override;
	bool ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) override;
	void reset() override;

private:
	uint8_t nPRGBankSelectLo = 0x00;
	uint8_t nPRGBankSelectHi = 0x00;
};


class Mapper_003 : public Mapper
{
public:
	Mapper_003(uint8_t prgBanks, uint8_t chrBanks);
	~Mapper_003();

	bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) override;
	bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) override;
	bool ppuMapRead(uint16_t addr, uint32_t &mapped_addr) override;
	bool ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) override;
	void reset() override;

private:
	uint8_t nCHRBankSelect = 0x00;


};


class Mapper_004 : public Mapper
{
public:
	Mapper_004(uint8_t prgBanks, uint8_t chrBanks);
	~Mapper_004();

	bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) override;
	bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) override;
	bool ppuMapRead(uint16_t addr, uint32_t &mapped_addr) override;
	bool ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) override;
	void reset() override;

	bool irqState() override;
	void irqClear() override;

	void scanline() override;
	MIRROR mirror() override;

private:
	// Control variables
	uint8_t nTargetRegister = 0x00;
	bool bPRGBankMode = false;
	bool bCHRInversion = false;
	MIRROR mirrormode = MIRROR::HORIZONTAL;

	uint32_t pRegister[8];
	uint32_t pCHRBank[8];
	uint32_t pPRGBank[4];

	bool bIRQActive = false;
	bool bIRQEnable = false;
	bool bIRQUpdate = false;
	uint16_t nIRQCounter = 0x0000;
	uint16_t nIRQReload = 0x0000;

	std::vector<uint8_t> vRAMStatic;
};


class Mapper_066 :	public Mapper
{
public:
	Mapper_066(uint8_t prgBanks, uint8_t chrBanks);
	~Mapper_066();

	bool cpuMapRead(uint16_t addr, uint32_t &mapped_addr, uint8_t &data) override;
	bool cpuMapWrite(uint16_t addr, uint32_t &mapped_addr, uint8_t data = 0) override;
	bool ppuMapRead(uint16_t addr, uint32_t &mapped_addr) override;
	bool ppuMapWrite(uint16_t addr, uint32_t &mapped_addr) override;
	void reset() override;

private:
	uint8_t nCHRBankSelect = 0x00;
	uint8_t nPRGBankSelect = 0x00;
};


class Cartridge
{
public:	
	Cartridge(const std::string& sFileName);
	~Cartridge();


public:
	bool ImageValid();

	

private:
	bool bImageValid = false;
	MIRROR hw_mirror = HORIZONTAL;

	uint8_t nMapperID = 0;
	uint8_t nPRGBanks = 0;
	uint8_t nCHRBanks = 0;

	std::vector<uint8_t> vPRGMemory;
	std::vector<uint8_t> vCHRMemory;

	std::shared_ptr<Mapper> pMapper;

public:
	// Communication with Main Bus
	bool cpuRead(uint16_t addr, uint8_t &data);
	bool cpuWrite(uint16_t addr, uint8_t data);

	// Communication with PPU Bus
	bool ppuRead(uint16_t addr, uint8_t &data);
	bool ppuWrite(uint16_t addr, uint8_t data);

	// Permits system rest of mapper to know state
	void reset();

	// Get Mirror configuration
	MIRROR Mirror();

	std::shared_ptr<Mapper> GetMapper();
};


class Bus;


// The 6502 Emulation Class. This is it!
class cpu6502
{
public:
	cpu6502();
	~cpu6502();

public:
	// CPU Core registers, exposed as public here for ease of access from external
	// examinors. This is all the 6502 has.
	uint8_t  a      = 0x00;		// Accumulator Register
	uint8_t  x      = 0x00;		// X Register
	uint8_t  y      = 0x00;		// Y Register
	uint8_t  stkp   = 0x00;		// Stack Pointer (points to location on bus)
	uint16_t pc     = 0x0000;	// Program Counter
	uint8_t  status = 0x00;		// Status Register
	
	// External event functions. In hardware these represent pins that are asserted
	// to produce a change in state.
	void reset();	// Reset Interrupt - Forces CPU into known state
	void irq();		// Interrupt Request - Executes an instruction at a specific location
	void nmi();		// Non-Maskable Interrupt Request - As above, but cannot be disabled
	void clock();	// Perform one clock cycle's worth of update

	// Indicates the current instruction has completed by returning true. This is
	// a utility function to enable "step-by-step" execution, without manually 
	// clocking every cycle
	bool complete();

	// Link this CPU to a communications bus
	void ConnectBus(Bus *n) { bus = n; }

	// Produces a map of strings, with keys equivalent to instruction start locations
	// in memory, for the specified address range
	std::map<uint16_t, std::string> disassemble(uint16_t nStart, uint16_t nStop);

	// The status register stores 8 flags. Ive enumerated these here for ease
	// of access. You can access the status register directly since its public.
	// The bits have different interpretations depending upon the context and 
	// instruction being executed.
	enum FLAGS6502
	{
		C = (1 << 0),	// Carry Bit
		Z = (1 << 1),	// Zero
		I = (1 << 2),	// Disable Interrupts
		D = (1 << 3),	// Decimal Mode (unused in this implementation)
		B = (1 << 4),	// Break
		U = (1 << 5),	// Unused
		V = (1 << 6),	// Overflow
		N = (1 << 7),	// Negative
	};

private:
	// Convenience functions to access status register
	uint8_t GetFlag(FLAGS6502 f);
	void    SetFlag(FLAGS6502 f, bool v);
	
	// Assisstive variables to facilitate emulation
	uint8_t  fetched     = 0x00;   // Represents the working input value to the ALU
	uint16_t temp        = 0x0000; // A convenience variable used everywhere
	uint16_t addr_abs    = 0x0000; // All used memory addresses end up in here
	uint16_t addr_rel    = 0x00;   // Represents absolute address following a branch
	uint8_t  opcode      = 0x00;   // Is the instruction byte
	uint8_t  cycles      = 0;	   // Counts how many cycles the instruction has remaining
	uint32_t clock_count = 0;	   // A global accumulation of the number of clocks

	// Linkage to the communications bus
	Bus     *bus = nullptr;
	uint8_t read(uint16_t a);
	void    write(uint16_t a, uint8_t d);

	// The read location of data can come from two sources, a memory address, or
	// its immediately available as part of the instruction. This function decides
	// depending on address mode of instruction byte
	uint8_t fetch();

	// This structure and the following vector are used to compile and store
	// the opcode translation table. The 6502 can effectively have 256
	// different instructions. Each of these are stored in a table in numerical
	// order so they can be looked up easily, with no decoding required.
	// Each table entry holds:
	//	Pneumonic : A textual representation of the instruction (used for disassembly)
	//	Opcode Function: A function pointer to the implementation of the opcode
	//	Opcode Address Mode : A function pointer to the implementation of the 
    //						  addressing mechanism used by the instruction
	//	Cycle Count : An integer that represents the base number of clock cycles the
	//				  CPU requires to perform the instruction

	struct INSTRUCTION
	{
		std::string name;		
		uint8_t     (cpu6502::*operate )(void) = nullptr;
		uint8_t     (cpu6502::*addrmode)(void) = nullptr;
		uint8_t     cycles = 0;
	};

	std::vector<INSTRUCTION> lookup;
	
private: 
	// Addressing Modes =============================================
	// The 6502 has a variety of addressing modes to access data in 
	// memory, some of which are direct and some are indirect (like
	// pointers in C++). Each opcode contains information about which
	// addressing mode should be employed to facilitate the 
	// instruction, in regards to where it reads/writes the data it
	// uses. The address mode changes the number of bytes that
	// makes up the full instruction, so we implement addressing
	// before executing the instruction, to make sure the program
	// counter is at the correct location, the instruction is
	// primed with the addresses it needs, and the number of clock
	// cycles the instruction requires is calculated. These functions
	// may adjust the number of cycles required depending upon where
	// and how the memory is accessed, so they return the required
	// adjustment.

	uint8_t IMP();	uint8_t IMM();	
	uint8_t ZP0();	uint8_t ZPX();	
	uint8_t ZPY();	uint8_t REL();
	uint8_t ABS();	uint8_t ABX();	
	uint8_t ABY();	uint8_t IND();	
	uint8_t IZX();	uint8_t IZY();

private: 
	// Opcodes ======================================================
	// There are 56 "legitimate" opcodes provided by the 6502 CPU. I
	// have not modelled "unofficial" opcodes. As each opcode is 
	// defined by 1 byte, there are potentially 256 possible codes.
	// Codes are not used in a "switch case" style on a processor,
	// instead they are repsonisble for switching individual parts of
	// CPU circuits on and off. The opcodes listed here are official, 
	// meaning that the functionality of the chip when provided with
	// these codes is as the developers intended it to be. Unofficial
	// codes will of course also influence the CPU circuitry in 
	// interesting ways, and can be exploited to gain additional
	// functionality!
	//
	// These functions return 0 normally, but some are capable of
	// requiring more clock cycles when executed under certain
	// conditions combined with certain addressing modes. If that is 
	// the case, they return 1.
	//
	// I have included detailed explanations of each function in 
	// the class implementation file. Note they are listed in
	// alphabetical order here for ease of finding.

	uint8_t ADC();	uint8_t AND();	uint8_t ASL();	uint8_t BCC();
	uint8_t BCS();	uint8_t BEQ();	uint8_t BIT();	uint8_t BMI();
	uint8_t BNE();	uint8_t BPL();	uint8_t BRK();	uint8_t BVC();
	uint8_t BVS();	uint8_t CLC();	uint8_t CLD();	uint8_t CLI();
	uint8_t CLV();	uint8_t CMP();	uint8_t CPX();	uint8_t CPY();
	uint8_t DEC();	uint8_t DEX();	uint8_t DEY();	uint8_t EOR();
	uint8_t INC();	uint8_t INX();	uint8_t INY();	uint8_t JMP();
	uint8_t JSR();	uint8_t LDA();	uint8_t LDX();	uint8_t LDY();
	uint8_t LSR();	uint8_t NOP();	uint8_t ORA();	uint8_t PHA();
	uint8_t PHP();	uint8_t PLA();	uint8_t PLP();	uint8_t ROL();
	uint8_t ROR();	uint8_t RTI();	uint8_t RTS();	uint8_t SBC();
	uint8_t SEC();	uint8_t SED();	uint8_t SEI();	uint8_t STA();
	uint8_t STX();	uint8_t STY();	uint8_t TAX();	uint8_t TAY();
	uint8_t TSX();	uint8_t TXA();	uint8_t TXS();	uint8_t TYA();

	// I capture all "unofficial" opcodes with this function. It is
	// functionally identical to a NOP
	uint8_t XXX();

#ifdef LOGMODE
private:
	FILE* logfile = nullptr;
#endif
};

// End of File - Jx9

class ppu2C02
{
public:
	ppu2C02();
	~ppu2C02();

private:		
	uint8_t     tblName[2][1024];
	uint8_t     tblPattern[2][4096];
	uint8_t		tblPalette[32];

private:
	kairo::Pixel  palScreen[0x40];
	// In Video
	// kairo::Sprite sprScreen = kairo::Sprite(256, 240);
	// kairo::Sprite sprNameTable[2] = { kairo::Sprite(256, 240), kairo::Sprite(256, 240) };
	// kairo::Sprite sprPatternTable[2] = { kairo::Sprite(128, 128), kairo::Sprite(128, 128) };

	// Changed To for API breaking subsequent PGE Update
	kairo::Sprite* sprScreen;
	kairo::Sprite* sprNameTable[2];
	kairo::Sprite* sprPatternTable[2];

public:
	// Debugging Utilities
	kairo::Sprite& GetScreen();
	kairo::Sprite& GetNameTable(uint8_t i);
	kairo::Sprite& GetPatternTable(uint8_t i, uint8_t palette);
	kairo::Pixel& GetColourFromPaletteRam(uint8_t palette, uint8_t pixel);
	bool frame_complete = false;

private:

	union PPUSTATUS
	{
		struct
		{
			uint8_t unused : 5;
			uint8_t sprite_overflow : 1;
			uint8_t sprite_zero_hit : 1;
			uint8_t vertical_blank : 1;
		};

		uint8_t reg;
	} status;


	union PPUMASK
	{
		struct
		{
			uint8_t grayscale : 1;
			uint8_t render_background_left : 1;
			uint8_t render_sprites_left : 1;
			uint8_t render_background : 1;
			uint8_t render_sprites : 1;
			uint8_t enhance_red : 1;
			uint8_t enhance_green : 1;
			uint8_t enhance_blue : 1;
		};

		uint8_t reg;
	} mask;

	union PPUCTRL
	{
		struct
		{
			uint8_t nametable_x : 1;
			uint8_t nametable_y : 1;
			uint8_t increment_mode : 1;
			uint8_t pattern_sprite : 1;
			uint8_t pattern_background : 1;
			uint8_t sprite_size : 1;
			uint8_t slave_mode : 1; // unused
			uint8_t enable_nmi : 1;
		};

		uint8_t reg;
	} control;

	union loopy_register
	{
		// Credit to Loopy for working this out :D
		struct
		{

			uint16_t coarse_x : 5;
			uint16_t coarse_y : 5;
			uint16_t nametable_x : 1;
			uint16_t nametable_y : 1;
			uint16_t fine_y : 3;
			uint16_t unused : 1;
		};

		uint16_t reg = 0x0000;
	};
	
	
	loopy_register vram_addr; // Active "pointer" address into nametable to extract background tile info
	loopy_register tram_addr; // Temporary store of information to be "transferred" into "pointer" at various times

	// Pixel offset horizontally
	uint8_t fine_x = 0x00;

	// Internal communications
	uint8_t address_latch = 0x00;
	uint8_t ppu_data_buffer = 0x00;

	// Pixel "dot" position information
	int16_t scanline = 0;
	int16_t cycle = 0;
	bool odd_frame = false;

	// Background rendering =========================================
	uint8_t bg_next_tile_id     = 0x00;
	uint8_t bg_next_tile_attrib = 0x00;
	uint8_t bg_next_tile_lsb    = 0x00;
	uint8_t bg_next_tile_msb    = 0x00;
	uint16_t bg_shifter_pattern_lo = 0x0000;
	uint16_t bg_shifter_pattern_hi = 0x0000;
	uint16_t bg_shifter_attrib_lo  = 0x0000;
	uint16_t bg_shifter_attrib_hi  = 0x0000;


	// Foreground "Sprite" rendering ================================
	// The OAM is an additional memory internal to the PPU. It is
	// not connected via the any bus. It stores the locations of
	// 64off 8x8 (or 8x16) tiles to be drawn on the next frame.
	struct sObjectAttributeEntry
	{
		uint8_t y;			// Y position of sprite
		uint8_t id;			// ID of tile from pattern memory
		uint8_t attribute;	// Flags define how sprite should be rendered
		uint8_t x;			// X position of sprite
	} OAM[64];

	// A register to store the address when the CPU manually communicates
	// with OAM via PPU registers. This is not commonly used because it 
	// is very slow, and instead a 256-Byte DMA transfer is used. See
	// the Bus header for a description of this.
	uint8_t oam_addr = 0x00;


	sObjectAttributeEntry spriteScanline[8];
	uint8_t sprite_count;
	uint8_t sprite_shifter_pattern_lo[8];
	uint8_t sprite_shifter_pattern_hi[8];

	// Sprite Zero Collision Flags
	bool bSpriteZeroHitPossible = false;
	bool bSpriteZeroBeingRendered = false;

	// The OAM is conveniently package above to work with, but the DMA
    // mechanism will need access to it for writing one byute at a time
public:
	uint8_t* pOAM = (uint8_t*)OAM;


public:
	// Communications with Main Bus
	uint8_t cpuRead(uint16_t addr, bool rdonly = false);
	void    cpuWrite(uint16_t addr, uint8_t  data);

	// Communications with PPU Bus
	uint8_t ppuRead(uint16_t addr, bool rdonly = false);
	void    ppuWrite(uint16_t addr, uint8_t data);

private:
	// The Cartridge or "GamePak"
	std::shared_ptr<Cartridge> cart;

public:
	// Interface
	void ConnectCartridge(const std::shared_ptr<Cartridge>& cartridge);
	void clock();
	void reset();
	bool nmi = false;
	bool scanline_trigger = false;
};


class apu2A03
{
public:
	apu2A03();
	~apu2A03();

public:
	void cpuWrite(uint16_t addr, uint8_t data);
	uint8_t cpuRead(uint16_t addr);
	void clock();
	void reset();

	double GetOutputSample();

private:
	uint32_t frame_clock_counter = 0;
	uint32_t clock_counter = 0;
	bool bUseRawMode = false;

private:
	static uint8_t length_table[];

private:
	
	// Sequencer Module
	// ~~~~~~~~~~~~~~~~
	// The purpose of the sequencer is to output a '1' after a given 
	// interval. It does this by counting down from a start value,
	// when that value is < 0, it gets reset, and an internal "rotary"
	// buffer is shifted. The nature of ths shifted pattern is different
	// depending upon the channel, or module that requires sequencing.
	// For example, the square wave channels simply rotate the preset
	// sequence, but the noise channel needs to generate pseudo-random
	// outputs originating from the preset sequence.
	//
	// Consider a square wave channel. A preset sequence of 01010101
	// will output a 1 more freqently than 00010001, assuming we 
	// always output the LSB. The speed of this output is also
	// governed by the timer counting down. The frequency is higher
	// for small timer values, and lower for larger. Increasing
	// the frequency of the output potentially increases the
	// audible frequency. In fact, this is how the pulse channels
	// fundamentally work. A "duty cycle" shape is loaded into the
	// sequencer and the timer is used to vary the pitch, yielding 
	// notes.

	struct sequencer
	{
		uint32_t sequence = 0x00000000;
		uint32_t new_sequence = 0x00000000;
		uint16_t timer = 0x0000;
		uint16_t reload = 0x0000;
		uint8_t output = 0x00;

		// Pass in a lambda function to manipulate the sequence as required
		// by the owner of this sequencer module
		uint8_t clock(bool bEnable, std::function<void(uint32_t &s)> funcManip)
		{
			if (bEnable)
			{
				timer--;
				if (timer == 0xFFFF)
				{
					timer = reload;
					funcManip(sequence);
					output = sequence & 0x00000001;
				}
			}
			return output;
		}
	};

	struct lengthcounter
	{
		uint8_t counter = 0x00;
		uint8_t clock(bool bEnable, bool bHalt)
		{
			if (!bEnable)
				counter = 0;
			else
				if (counter > 0 && !bHalt)
					counter--;
			return counter;
		}
	};

	struct envelope
	{
		void clock(bool bLoop)
		{
			if (!start)
			{
				if (divider_count == 0)
				{
					divider_count = volume;

					if (decay_count == 0)
					{
						if (bLoop)
						{
							decay_count = 15;
						}

					}
					else
						decay_count--;
				}
				else
					divider_count--;
			}
			else
			{
				start = false;
				decay_count = 15;
				divider_count = volume;
			}

			if (disable)
			{
				output = volume;
			}
			else
			{
				output = decay_count;
			}
		}

		bool start = false;
		bool disable = false;
		uint16_t divider_count = 0;
		uint16_t volume = 0;
		uint16_t output = 0;
		uint16_t decay_count = 0;
	};


	struct oscpulse
	{
		double frequency = 0;
		double dutycycle = 0;
		double amplitude = 1;
		double pi = 3.14159;
		double harmonics = 20;

		double sample(double t)
		{
			double a = 0;
			double b = 0;
			double p = dutycycle * 2.0 * pi;

			auto approxsin = [](double t)
			{
				double j = t * 0.15915;
				j = j - (int)j;
				return 20.785 * j * (j - 0.5) * (j - 1.0);
			};

			for (double n = 1; n < harmonics; n++)
			{
				double c = n * frequency * 2.0 * pi * t;
				a += -approxsin(c) / n;
				b += -approxsin(c - p * n) / n;

				//a += -sin(c) / n;
				//b += -sin(c - p * n) / n;
			}

			return (2.0 * amplitude / pi) * (a - b);
		}
	};

	struct sweeper
	{
		bool enabled = false;
		bool down = false;
		bool reload = false;
		uint8_t shift = 0x00;
		uint8_t timer = 0x00;
		uint8_t period = 0x00;
		uint16_t change = 0;
		bool mute = false;

		void track(uint16_t &target)
		{
			if (enabled)
			{
				change = target >> shift;
				mute = (target < 8) || (target > 0x7FF);
			}
		}

		bool clock(uint16_t &target, bool channel)
		{
			bool changed = false;
			if (timer == 0 && enabled && shift > 0 && !mute)
			{
				if (target >= 8 && change < 0x07FF)
				{
					if (down)
					{
						target -= change - channel;
					}
					else
					{
						target += change;
					}
					changed = true;
				}
			}

			//if (enabled)
			{
				if (timer == 0 || reload)
				{
					timer = period;
					reload = false;
				}
				else
					timer--;

				mute = (target < 8) || (target > 0x7FF);
			}

			return changed;
		}
	};

	double dGlobalTime = 0.0;

	// Square Wave Pulse Channel 1
	bool pulse1_enable = false;
	bool pulse1_halt = false;
	double pulse1_sample = 0.0;
	double pulse1_output = 0.0;
	sequencer pulse1_seq;
	oscpulse pulse1_osc;
	envelope pulse1_env;
	lengthcounter pulse1_lc;
	sweeper pulse1_sweep;

	// Square Wave Pulse Channel 2
	bool pulse2_enable = false;
	bool pulse2_halt = false;
	double pulse2_sample = 0.0;
	double pulse2_output = 0.0;
	sequencer pulse2_seq;
	oscpulse pulse2_osc;
	envelope pulse2_env;
	lengthcounter pulse2_lc;
	sweeper pulse2_sweep;

	// Noise Channel
	bool noise_enable = false;
	bool noise_halt = false;
	envelope noise_env;
	lengthcounter noise_lc;
	sequencer noise_seq;
	double noise_sample = 0;
	double noise_output = 0;


public:
	uint16_t pulse1_visual = 0;
	uint16_t pulse2_visual = 0;
	uint16_t noise_visual = 0;
	uint16_t triangle_visual = 0;
	
};


class Bus
{
public:
	Bus();
	~Bus();

public: // Devices on Main Bus

	// The 6502 derived processor
	cpu6502 cpu;	
	// The 2C02 Picture Processing Unit
	ppu2C02 ppu;
	// The "2A03" Audio Processing Unit
	apu2A03 apu;
	// The Cartridge or "GamePak"
	std::shared_ptr<Cartridge> cart;
	// 2KB of RAM
	uint8_t cpuRam[2048];
	// Controllers
	uint8_t controller[2];

	// Synchronisation with system Audio
public:
	void SetSampleFrequency(uint32_t sample_rate);
	double dAudioSample = 0.0;

private:
	double dAudioTime = 0.0;
	double dAudioGlobalTime = 0.0;
	double dAudioTimePerNESClock = 0.0;
	double dAudioTimePerSystemSample = 0.0f;


public: // Main Bus Read & Write
	void    cpuWrite(uint16_t addr, uint8_t data);
	uint8_t cpuRead(uint16_t addr, bool bReadOnly = false);

private:
	// A count of how many clocks have passed
	uint32_t nSystemClockCounter = 0;
	// Internal cache of controller state
	uint8_t controller_state[2];

private:
	// A simple form of Direct Memory Access is used to swiftly
	// transfer data from CPU bus memory into the OAM memory. It would
	// take too long to sensibly do this manually using a CPU loop, so
	// the program prepares a page of memory with the sprite info required
	// for the next frame and initiates a DMA transfer. This suspends the
	// CPU momentarily while the PPU gets sent data at PPU clock speeds.
	// Note here, that dma_page and dma_addr form a 16-bit address in 
	// the CPU bus address space
	uint8_t dma_page = 0x00;
	uint8_t dma_addr = 0x00;
	uint8_t dma_data = 0x00;

	// DMA transfers need to be timed accurately. In principle it takes
	// 512 cycles to read and write the 256 bytes of the OAM memory, a
	// read followed by a write. However, the CPU needs to be on an "even"
	// clock cycle, so a dummy cycle of idleness may be required
	bool dma_dummy = true;

	// Finally a flag to indicate that a DMA transfer is happening
	bool dma_transfer = false;

public: // System Interface
	// Connects a cartridge object to the internal buses
	void insertCartridge(const std::shared_ptr<Cartridge>& cartridge);
	// Resets the system
	void reset();
	// Clocks the system - a single whole system tick
	bool clock();
};


}
