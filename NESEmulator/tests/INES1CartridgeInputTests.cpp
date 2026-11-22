// Portable iNES1 safety tests against the exact header/payload reader used by
// Cartridge.cpp. No homebrew, game window, Metal/OpenGL, ROM images, mapper
// modules, or third-party media are required. All sample bytes are generated
// here and may be freely reused for teaching/debugging.
#include "../INES1Header.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <string>

namespace
{
    using Bytes = std::array<std::uint8_t, 16u>;

    [[nodiscard]] Bytes Header(std::uint8_t prgBanks = 1u,
        std::uint8_t chrBanks = 1u, std::uint8_t flags6 = 0u,
        std::uint8_t flags7 = 0u)
    {
        Bytes bytes{};
        bytes[0] = 'N'; bytes[1] = 'E'; bytes[2] = 'S'; bytes[3] = 0x1au;
        bytes[4] = prgBanks;
        bytes[5] = chrBanks;
        bytes[6] = flags6;
        bytes[7] = flags7;
        return bytes;
    }

    [[nodiscard]] std::string RawRom(const Bytes& h,
        std::size_t trainerBytes, std::size_t prgBytes, std::size_t chrBytes)
    {
        std::string bytes(reinterpret_cast<const char*>(h.data()), h.size());
        bytes.append(trainerBytes, static_cast<char>(0x3d));
        bytes.append(prgBytes, static_cast<char>(0x51));
        bytes.append(chrBytes, static_cast<char>(0xa6));
        return bytes;
    }

    void Require(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    template<class Fn>
    void Reject(Fn&& action, const char* label)
    {
        bool rejected = false;
        try { action(); }
        catch (const std::invalid_argument&) { rejected = true; }
        Require(rejected, label);
    }

    [[nodiscard]] kairo::nes::detail::INES1RomPayload ParseRaw(
        const std::string& bytes)
    {
        std::istringstream input(bytes, std::ios::in | std::ios::binary);
        return kairo::nes::detail::ReadINES1Rom(input);
    }
}

int main()
{
    try
    {
        using kairo::nes::detail::ParseINES1Header;
        const auto standard = ParseINES1Header(Header());
        Require(standard.Mapper == 0u && standard.PRGBanks == 1u &&
            standard.CHRBanks == 1u && standard.PRGBytes == 16384u &&
            standard.CHRFileBytes == 8192u &&
            standard.CHRStorageBytes == 8192u,
            "NROM 16K PRG + 8K CHR header decoding failed");

        const auto full = ParseRaw(RawRom(Header(), 0u, 16384u, 8192u));
        Require(full.PRG.size() == 16384u && full.CHR.size() == 8192u &&
            full.PRG.front() == 0x51u && full.CHR.front() == 0xa6u,
            "Complete iNES ROM image was not decoded at correct offsets");

        // A CHR bank count of zero means 8 KiB RAM. The file needs NO CHR
        // payload bytes; old Cartridge.cpp incorrectly attempted to read 8K.
        const auto chrRAM = ParseRaw(RawRom(Header(1u, 0u), 0u, 16384u, 0u));
        Require(chrRAM.Header.CHRFileBytes == 0u &&
            chrRAM.CHR.size() == 8192u &&
            std::all_of(chrRAM.CHR.begin(), chrRAM.CHR.end(),
                [](std::uint8_t byte) { return byte == 0u; }),
            "CHR-RAM-only cartridge did not initialize zeroed 8K RAM");

        const auto trainerHeader = Header(2u, 1u, 0x25u, 0x40u);
        // mapper = (0x40 | 0x2) = 66, flags6 bit2 trainer, bit0 vertical.
        const auto trainer = ParseRaw(RawRom(trainerHeader, 512u,
            32768u, 8192u));
        Require(trainer.Header.Mapper == 66u &&
            trainer.Header.TrainerPresent && trainer.Header.VerticalMirroring &&
            trainer.PRG.size() == 32768u && trainer.PRG.front() == 0x51u &&
            trainer.CHR.front() == 0xa6u,
            "Trainer offset/mapper-66/vertical mirror metadata was wrong");

        const auto maxHeader = ParseINES1Header(Header(255u, 255u));
        Require(maxHeader.PRGBytes == 255u * 16384u &&
            maxHeader.CHRFileBytes == 255u * 8192u,
            "Upper iNES1 bank limits wrapped the byte count");

        auto badMagic = Header();
        badMagic[2] = 'X';
        Reject([&] { (void)ParseINES1Header(badMagic); },
            "Invalid NES signature accepted");
        Reject([&] { (void)ParseINES1Header(Header(0u, 1u)); },
            "Cartridge with no PRG accepted");
        Reject([&] { (void)ParseINES1Header(Header(1u, 1u, 0u, 0x08u)); },
            "NES2 header misinterpreted as legacy iNES1");
        Reject([&] { (void)ParseINES1Header(Header(1u, 1u, 0x08u)); },
            "Unsupported four-screen nametable wiring accepted");
        Reject([&] { (void)ParseINES1Header(Header(1u, 1u, 0x50u)); },
            "Unsupported mapper 5 was accepted");

        Reject([&] { (void)ParseRaw(std::string("NES")); },
            "Truncated header accepted");
        Reject([&] {
            (void)ParseRaw(RawRom(Header(1u, 1u, 0x04u),
                511u, 0u, 0u));
        }, "Truncated 512-byte trainer accepted");
        Reject([&] { (void)ParseRaw(RawRom(Header(), 0u, 16383u, 0u)); },
            "Truncated PRG bytes accepted");
        Reject([&] { (void)ParseRaw(RawRom(Header(), 0u, 16384u, 8191u)); },
            "Truncated CHR bytes accepted");

        // A complete legacy image may have harmless trailing file bytes.
        const auto trailing = ParseRaw(RawRom(Header(), 0u, 16384u, 8192u)
            + std::string(12u, 'P'));
        Require(trailing.CHR.back() == 0xa6u,
            "Trailing padding changed the decoded CHR window");

        std::puts("Portable iNES1 cartridge header and payload tests passed");
        return 0;
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "iNES1 input regression: %s\n", e.what());
        return 1;
    }
}
