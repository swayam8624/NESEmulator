#pragma once
#include <cstdint>
#include <memory>

#include "PixelGameEngine.h"

#include "Cartridge.h"

class ppu2C02
{
public:
    ppu2C02();
    ~ppu2C02();

private:
    uint8_t     tblName[2][1024];
    uint8_t     tblPattern[2][4096];
    uint8_t        tblPalette[32];

private:
    kairo::Pixel  palScreen[0x40];
    // In Video
    // Sprite sprScreen = Sprite(256, 240);
    // Sprite sprNameTable[2] = { Sprite(256, 240), Sprite(256, 240) };
    // Sprite sprPatternTable[2] = { Sprite(128, 128), Sprite(128, 128) };

    // Changed To for API breaking subsequent PGE Update
    kairo::Sprite* sprScreen;
    kairo::Sprite* sprNameTable[2];
    kairo::Sprite* sprPatternTable[2];

public:
    // Debugging Utilities
    kairo::Sprite& GetScreen();
    kairo::Sprite& GetNameTable(uint8_t i);
    kairo::Sprite& GetPatternTable(uint8_t i);
    bool frame_complete = false;

private:
    int16_t scanline = 0;
    int16_t cycle = 0;
    

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
};
