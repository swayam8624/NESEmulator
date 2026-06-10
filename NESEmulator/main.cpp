#include <iostream>
#include <sstream>
#include <deque>
#include <array>
#include <utility>

import Kairo.NES;

#define kairo_PGE_APPLICATION
#include "KairoPixelGameEngine.cppm"

class Demo_KairoNES : public kairo::PixelGameEngine
{
public:
	explicit Demo_KairoNES(std::string romPath)
		: selectedRomPath(std::move(romPath))
	{
		sAppName = "KairoNES";
	}

private: 
	// The NES
	Bus nes;
	std::shared_ptr<Cartridge> cart;
	std::string selectedRomPath;
	bool bEmulationRun = true;
	float fResidualTime = 0.0f;

	uint8_t nSelectedPalette = 0x00;

	std::list<uint16_t> audio[4];
	float fAccumulatedTime = 0.0f;

private: 
	// Support Utilities
	std::map<uint16_t, std::string> mapAsm;

	std::string hex(uint32_t n, uint8_t d)
	{
		std::string s(d, '0');
		for (int i = d - 1; i >= 0; i--, n >>= 4)
			s[i] = "0123456789ABCDEF"[n & 0xF];
		return s;
	};

	void DrawRam(int x, int y, uint16_t nAddr, int nRows, int nColumns)
	{
		int nRamX = x, nRamY = y;
		for (int row = 0; row < nRows; row++)
		{
			std::string sOffset = "$" + hex(nAddr, 4) + ":";
			for (int col = 0; col < nColumns; col++)
			{
				sOffset += " " + hex(nes.cpuRead(nAddr, true), 2);
				nAddr += 1;
			}
			DrawString(nRamX, nRamY, sOffset);
			nRamY += 10;
		}
	}

	void DrawCpu(int x, int y)
	{
		std::string status = "STATUS: ";
		DrawString(x , y , "STATUS:", kairo::WHITE);
		DrawString(x  + 64, y, "N", nes.cpu.status & cpu6502::N ? kairo::GREEN : kairo::RED);
		DrawString(x  + 80, y , "V", nes.cpu.status & cpu6502::V ? kairo::GREEN : kairo::RED);
		DrawString(x  + 96, y , "-", nes.cpu.status & cpu6502::U ? kairo::GREEN : kairo::RED);
		DrawString(x  + 112, y , "B", nes.cpu.status & cpu6502::B ? kairo::GREEN : kairo::RED);
		DrawString(x  + 128, y , "D", nes.cpu.status & cpu6502::D ? kairo::GREEN : kairo::RED);
		DrawString(x  + 144, y , "I", nes.cpu.status & cpu6502::I ? kairo::GREEN : kairo::RED);
		DrawString(x  + 160, y , "Z", nes.cpu.status & cpu6502::Z ? kairo::GREEN : kairo::RED);
		DrawString(x  + 178, y , "C", nes.cpu.status & cpu6502::C ? kairo::GREEN : kairo::RED);
		DrawString(x , y + 10, "PC: $" + hex(nes.cpu.pc, 4));
		DrawString(x , y + 20, "A: $" +  hex(nes.cpu.a, 2) + "  [" + std::to_string(nes.cpu.a) + "]");
		DrawString(x , y + 30, "X: $" +  hex(nes.cpu.x, 2) + "  [" + std::to_string(nes.cpu.x) + "]");
		DrawString(x , y + 40, "Y: $" +  hex(nes.cpu.y, 2) + "  [" + std::to_string(nes.cpu.y) + "]");
		DrawString(x , y + 50, "Stack P: $" + hex(nes.cpu.stkp, 4));
	}

	void DrawCode(int x, int y, int nLines)
	{
		auto it_a = mapAsm.find(nes.cpu.pc);
		int nLineY = (nLines >> 1) * 10 + y;
		if (it_a != mapAsm.end())
		{
			DrawString(x, nLineY, (*it_a).second, kairo::CYAN);
			while (nLineY < (nLines * 10) + y)
			{
				nLineY += 10;
				if (++it_a != mapAsm.end())
				{
					DrawString(x, nLineY, (*it_a).second);
				}
			}
		}

		it_a = mapAsm.find(nes.cpu.pc);
		nLineY = (nLines >> 1) * 10 + y;
		if (it_a != mapAsm.end())
		{
			while (nLineY > y)
			{
				nLineY -= 10;
				if (--it_a != mapAsm.end())
				{
					DrawString(x, nLineY, (*it_a).second);
				}
			}
		}
	}

	void DrawAudio(int channel, int x, int y)
	{
		FillRect(x, y, 120, 120, kairo::BLACK);
		int i = 0;
		for (auto s : audio[channel])
		{
			Draw(x + i, y + (s >> (channel == 2 ? 5 : 4)), kairo::YELLOW);
			i++;
		}
	}

	bool HeldAny(std::initializer_list<kairo::Key> keys) const
	{
		for (const auto key : keys)
			if (GetKey(key).bHeld)
				return true;

		return false;
	}

	void PollController()
	{
		nes.controller[0] = 0x00;
		nes.controller[0] |= HeldAny({kairo::Key::J, kairo::Key::K}) ? 0x80 : 0x00; // A
		nes.controller[0] |= HeldAny({kairo::Key::L}) ? 0x40 : 0x00;               // B
		nes.controller[0] |= HeldAny({kairo::Key::SPACE}) ? 0x20 : 0x00;           // Select
		nes.controller[0] |= HeldAny({kairo::Key::ENTER}) ? 0x10 : 0x00;           // Start
		nes.controller[0] |= HeldAny({kairo::Key::UP, kairo::Key::W}) ? 0x08 : 0x00;
		nes.controller[0] |= HeldAny({kairo::Key::DOWN, kairo::Key::S}) ? 0x04 : 0x00;
		nes.controller[0] |= HeldAny({kairo::Key::LEFT, kairo::Key::A}) ? 0x02 : 0x00;
		nes.controller[0] |= HeldAny({kairo::Key::RIGHT, kairo::Key::D}) ? 0x01 : 0x00;
	}

	// This function is called by the underlying sound hardware
	// which runs in a different thread. It is automatically
	// synchronised with the sample rate of the sound card, and
	// expects a single "sample" to be returned, whcih ultimately
	// makes its way to your speakers, and then your ears, for that
	// lovely 8-bit bliss... but, that means we've some thread
	// handling to deal with, since we want both the PGE thread
	// and the sound system thread to interact with the emulator.

	static Demo_KairoNES* pInstance; // Static variable that will hold a pointer to "this"

	static float SoundOut(int nChannel, float fGlobalTime, float fTimeStep)
	{
		if (nChannel == 0)
		{
			while (!pInstance->nes.clock()) {};
			return static_cast<float>(pInstance->nes.dAudioSample);
		}
		else
			return 0.0f;
	}

	bool OnUserCreate() override
	{
		std::vector<std::string> romPaths;
		if (!selectedRomPath.empty())
			romPaths.push_back(selectedRomPath);

		romPaths.emplace_back("nestest.nes");
		romPaths.emplace_back("NESEmulator/nestest.nes");
		romPaths.emplace_back("../NESEmulator/nestest.nes");

		for (const auto& path : romPaths)
		{
			cart = std::make_shared<Cartridge>(path);
			if (cart->ImageValid())
				break;
		}
		
		if (!cart->ImageValid())
			return false;

		// Insert into NES
		nes.insertCartridge(cart);
					
		// Extract dissassembly
		//mapAsm = nes.cpu.disassemble(0x0000, 0xFFFF);

		

		for (int i = 0; i < 4; i++)
		{			
			for (int j = 0; j < 120; j++)
				audio[i].push_back(0);
		}
		
		// Reset NES
		nes.reset();
		pInstance = this;
		nes.SetSampleFrequency(44100);
		return true;
	}

	// We must play nicely now with the sound hardware, so unload
	// it when the application terminates
	bool OnUserDestroy() override
	{
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		EmulatorUpdateWithoutAudio(fElapsedTime);
		return true;
	}

	// This performs an emulation update but synced to audio, so it cant
	// perform stepping through code or frames. Essentially, it runs
	// the emulation in real time now, so only accepts "controller" input
	// and updates the display
	bool EmulatorUpdateWithAudio(float fElapsedTime)
	{
		// Sample audio channel output roughly once per frame
		fAccumulatedTime += fElapsedTime;
		if (fAccumulatedTime >= 1.0f / 60.0f)
		{
			fAccumulatedTime -= (1.0f / 60.0f);
			audio[0].pop_front();
			audio[0].push_back(nes.apu.pulse1_visual);
			audio[1].pop_front();
			audio[1].push_back(nes.apu.pulse2_visual);
			audio[2].pop_front();
			audio[2].push_back(nes.apu.noise_visual);
		}


		Clear(kairo::DARK_BLUE);

		PollController();

		if (GetKey(kairo::Key::R).bPressed) nes.reset();

		DrawCpu(516, 2);
		//DrawCode(516, 72, 26);

		// Draw OAM Contents (first 26 out of 64) ======================================
		/*for (int i = 0; i < 26; i++)
		{
			std::string s = hex(i, 2) + ": (" + std::to_string(nes.ppu.pOAM[i * 4 + 3])
				+ ", " + std::to_string(nes.ppu.pOAM[i * 4 + 0]) + ") "
				+ "ID: " + hex(nes.ppu.pOAM[i * 4 + 1], 2) +
				+" AT: " + hex(nes.ppu.pOAM[i * 4 + 2], 2);
			DrawString(516, 72 + i * 10, s);
		}*/

		// Draw AUDIO Channels
		DrawAudio(0, 520, 72);
		DrawAudio(1, 644, 72);
		DrawAudio(2, 520, 196);
		DrawAudio(3, 644, 196);

		// Draw Palettes & Pattern Tables ==============================================
		const int nSwatchSize = 6;
		for (int p = 0; p < 8; p++) // For each palette
			for(int s = 0; s < 4; s++) // For each index
				FillRect(516 + p * (nSwatchSize * 5) + s * nSwatchSize, 340, 
					nSwatchSize, nSwatchSize, nes.ppu.GetColourFromPaletteRam(p, s));
		
		// Draw selection reticule around selected palette
		DrawRect(516 + nSelectedPalette * (nSwatchSize * 5) - 1, 339, (nSwatchSize * 4), nSwatchSize, kairo::WHITE);

		// Generate Pattern Tables
		DrawSprite(516, 348, &nes.ppu.GetPatternTable(0, nSelectedPalette));
		DrawSprite(648, 348, &nes.ppu.GetPatternTable(1, nSelectedPalette));

		// Draw rendered output ========================================================
		DrawSprite(0, 0, &nes.ppu.GetScreen(), 2);
		return true;
	}

	// This performs emulation with no audio synchronisation, so it is just
	// as before, in all the previous videos
	bool EmulatorUpdateWithoutAudio(float fElapsedTime)
	{
		PollController();

		if (GetKey(kairo::Key::P).bPressed) bEmulationRun = !bEmulationRun;
		if (GetKey(kairo::Key::R).bPressed) nes.reset();

		if (bEmulationRun)
		{
			if (fResidualTime > 0.0f)
				fResidualTime -= fElapsedTime;
			else
			{
				fResidualTime += (1.0f / 60.0f) - fElapsedTime;
				do { nes.clock(); } while (!nes.ppu.frame_complete);
				nes.ppu.frame_complete = false;
			}
		}
		else
		{
			// Emulate code step-by-step
			if (GetKey(kairo::Key::C).bPressed)
			{
				// Clock enough times to execute a whole CPU instruction
				do { nes.clock(); } while (!nes.cpu.complete());
				// CPU clock runs slower than system clock, so it may be
				// complete for additional system clock cycles. Drain
				// those out
				do { nes.clock(); } while (nes.cpu.complete());
			}

			// Emulate one whole frame
			if (GetKey(kairo::Key::F).bPressed)
			{
				// Clock enough times to draw a single frame
				do { nes.clock(); } while (!nes.ppu.frame_complete);
				// Use residual clock cycles to complete current instruction
				do { nes.clock(); } while (!nes.cpu.complete());
				// Reset frame completion flag
				nes.ppu.frame_complete = false;
			}
		}

		Clear(kairo::BLACK);
		DrawSprite(0, 0, &nes.ppu.GetScreen());
		return true;
	}
};

// Provide implementation for our static pointer
Demo_KairoNES* Demo_KairoNES::pInstance = nullptr;

int main(int argc, char** argv)
{
	const std::string romPath = argc > 1 ? argv[1] : "";
	Demo_KairoNES demo(romPath);
	demo.Construct(256, 240, 3, 3);
	demo.Start();
	return 0;
}
