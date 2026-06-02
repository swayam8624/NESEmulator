#include "ppu2C02.h"

ppu2C02::ppu2C02()
{
    palScreen[0x00] = kairo::Pixel(84, 84, 84);
    palScreen[0x01] = kairo::Pixel(0, 30, 116);
    palScreen[0x02] = kairo::Pixel(8, 16, 144);
    palScreen[0x03] = kairo::Pixel(48, 0, 136);
    palScreen[0x04] = kairo::Pixel(68, 0, 100);
    palScreen[0x05] = kairo::Pixel(92, 0, 48);
    palScreen[0x06] = kairo::Pixel(84, 4, 0);
    palScreen[0x07] = kairo::Pixel(60, 24, 0);
    palScreen[0x08] = kairo::Pixel(32, 42, 0);
    palScreen[0x09] = kairo::Pixel(8, 58, 0);
    palScreen[0x0A] = kairo::Pixel(0, 64, 0);
    palScreen[0x0B] = kairo::Pixel(0, 60, 0);
    palScreen[0x0C] = kairo::Pixel(0, 50, 60);
    palScreen[0x0D] = kairo::Pixel(0, 0, 0);
    palScreen[0x0E] = kairo::Pixel(0, 0, 0);
    palScreen[0x0F] = kairo::Pixel(0, 0, 0);

    palScreen[0x10] = kairo::Pixel(152, 150, 152);
    palScreen[0x11] = kairo::Pixel(8, 76, 196);
    palScreen[0x12] = kairo::Pixel(48, 50, 236);
    palScreen[0x13] = kairo::Pixel(92, 30, 228);
    palScreen[0x14] = kairo::Pixel(136, 20, 176);
    palScreen[0x15] = kairo::Pixel(160, 20, 100);
    palScreen[0x16] = kairo::Pixel(152, 34, 32);
    palScreen[0x17] = kairo::Pixel(120, 60, 0);
    palScreen[0x18] = kairo::Pixel(84, 90, 0);
    palScreen[0x19] = kairo::Pixel(40, 114, 0);
    palScreen[0x1A] = kairo::Pixel(8, 124, 0);
    palScreen[0x1B] = kairo::Pixel(0, 118, 40);
    palScreen[0x1C] = kairo::Pixel(0, 102, 120);
    palScreen[0x1D] = kairo::Pixel(0, 0, 0);
    palScreen[0x1E] = kairo::Pixel(0, 0, 0);
    palScreen[0x1F] = kairo::Pixel(0, 0, 0);

    palScreen[0x20] = kairo::Pixel(236, 238, 236);
    palScreen[0x21] = kairo::Pixel(76, 154, 236);
    palScreen[0x22] = kairo::Pixel(120, 124, 236);
    palScreen[0x23] = kairo::Pixel(176, 98, 236);
    palScreen[0x24] = kairo::Pixel(228, 84, 236);
    palScreen[0x25] = kairo::Pixel(236, 88, 180);
    palScreen[0x26] = kairo::Pixel(236, 106, 100);
    palScreen[0x27] = kairo::Pixel(212, 136, 32);
    palScreen[0x28] = kairo::Pixel(160, 170, 0);
    palScreen[0x29] = kairo::Pixel(116, 196, 0);
    palScreen[0x2A] = kairo::Pixel(76, 208, 32);
    palScreen[0x2B] = kairo::Pixel(56, 204, 108);
    palScreen[0x2C] = kairo::Pixel(56, 180, 204);
    palScreen[0x2D] = kairo::Pixel(60, 60, 60);
    palScreen[0x2E] = kairo::Pixel(0, 0, 0);
    palScreen[0x2F] = kairo::Pixel(0, 0, 0);

    palScreen[0x30] = kairo::Pixel(236, 238, 236);
    palScreen[0x31] = kairo::Pixel(168, 204, 236);
    palScreen[0x32] = kairo::Pixel(188, 188, 236);
    palScreen[0x33] = kairo::Pixel(212, 178, 236);
    palScreen[0x34] = kairo::Pixel(236, 174, 236);
    palScreen[0x35] = kairo::Pixel(236, 174, 212);
    palScreen[0x36] = kairo::Pixel(236, 180, 176);
    palScreen[0x37] = kairo::Pixel(228, 196, 144);
    palScreen[0x38] = kairo::Pixel(204, 210, 120);
    palScreen[0x39] = kairo::Pixel(180, 222, 120);
    palScreen[0x3A] = kairo::Pixel(168, 226, 144);
    palScreen[0x3B] = kairo::Pixel(152, 226, 180);
    palScreen[0x3C] = kairo::Pixel(160, 214, 228);
    palScreen[0x3D] = kairo::Pixel(160, 162, 160);
    palScreen[0x3E] = kairo::Pixel(0, 0, 0);
    palScreen[0x3F] = kairo::Pixel(0, 0, 0);

    sprScreen = new kairo::Sprite(256, 240);
    sprNameTable[0] = new kairo::Sprite(256, 240);
    sprNameTable[1] = new kairo::Sprite(256, 240);
    sprPatternTable[0] = new kairo::Sprite(128, 128);
    sprPatternTable[1] = new kairo::Sprite(128, 128);
}


ppu2C02::~ppu2C02()
{
    delete sprScreen;
    delete sprNameTable[0];
    delete sprNameTable[1];
    delete sprPatternTable[0];
    delete sprPatternTable[1];
}

kairo::Sprite& ppu2C02::GetScreen()
{
    return *sprScreen;
}

kairo::Sprite & ppu2C02::GetNameTable(uint8_t i)
{
    return *sprNameTable[i];
}

kairo::Sprite & ppu2C02::GetPatternTable(uint8_t i)
{
    return *sprPatternTable[i];
}

uint8_t ppu2C02::cpuRead(uint16_t addr, bool rdonly)
{
    uint8_t data = 0x00;

    switch (addr)
    {
    case 0x0000: // Control
        break;
    case 0x0001: // Mask
        break;
    case 0x0002: // Status
        break;
    case 0x0003: // OAM Address
        break;
    case 0x0004: // OAM Data
        break;
    case 0x0005: // Scroll
        break;
    case 0x0006: // PPU Address
        break;
    case 0x0007: // PPU Data
        break;
    }

    return data;
}

void ppu2C02::cpuWrite(uint16_t addr, uint8_t data)
{
    switch (addr)
    {
    case 0x0000: // Control
        break;
    case 0x0001: // Mask
        break;
    case 0x0002: // Status
        break;
    case 0x0003: // OAM Address
        break;
    case 0x0004: // OAM Data
        break;
    case 0x0005: // Scroll
        break;
    case 0x0006: // PPU Address
        break;
    case 0x0007: // PPU Data
        break;
    }
}

uint8_t ppu2C02::ppuRead(uint16_t addr, bool rdonly)
{
    uint8_t data = 0x00;
    addr &= 0x3FFF;

    if (cart->ppuRead(addr, data))
    {

    }

    return data;
}

void ppu2C02::ppuWrite(uint16_t addr, uint8_t data)
{
    addr &= 0x3FFF;

    if (cart->ppuWrite(addr, data))
    {

    }
}

void ppu2C02::ConnectCartridge(const std::shared_ptr<Cartridge>& cartridge)
{
    this->cart = cartridge;
}

void ppu2C02::clock()
{

    // Fake some noise for now
    sprScreen->SetPixel(cycle - 1, scanline, palScreen[(rand() % 2) ? 0x3F : 0x30]);

    // Advance renderer - it never stops, it's relentless
    cycle++;
    if (cycle >= 341)
    {
        cycle = 0;
        scanline++;
        if (scanline >= 261)
        {
            scanline = -1;
            frame_complete = true;
        }
    }
}
