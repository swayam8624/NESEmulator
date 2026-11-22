#pragma once

// CPU-only iNES 1.0 header contract, shared by Cartridge.cpp and a portable
// regression test. ROM bytes and mapper registers are untrusted input: reject
// unsupported formats/mappers BEFORE any PRG/CHR vector allocation.
//
// A 16-byte header is NOT a ROM image. The caller must also read a complete
// optional 512-byte trainer, PRG payload and CHR payload. A zero CHR bank count
// means 8 KiB of CHR RAM, NOT 8 KiB to read from the file.
//
// NES 2.0 uses wider bank counts and extended mapper identity. KairoNES still
// uses uint8_t bank counts and uint8_t mapper IDs: rejecting NES 2.0 is safer
// than silently truncating sizes/banks into the wrong hardware configuration.

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <stdexcept>
#include <vector>

namespace kairo::nes::detail
{
    struct INES1Header final
    {
        std::uint8_t Mapper = 0;
        std::uint8_t PRGBanks = 0;
        std::uint8_t CHRBanks = 0;
        bool TrainerPresent = false;
        bool VerticalMirroring = false;
        std::size_t PRGBytes = 0;
        std::size_t CHRFileBytes = 0;
        std::size_t CHRStorageBytes = 0;
    };

    // This reader returns fully owned bounded byte vectors, never references
    // into the input stream. Cartridge uses them only after every expected
    // region was completely read, making short files an atomic rejection.
    struct INES1RomPayload final
    {
        INES1Header Header;
        std::vector<std::uint8_t> PRG;
        std::vector<std::uint8_t> CHR;
    };

    [[nodiscard]] inline INES1Header ParseINES1Header(
        const std::array<std::uint8_t, 16u>& bytes)
    {
        // Signature: 4E 45 53 1A ("NES" + MS-DOS EOF marker).
        if (bytes[0] != 0x4eu || bytes[1] != 0x45u ||
            bytes[2] != 0x53u || bytes[3] != 0x1au)
            throw std::invalid_argument("iNES header signature is missing.");

        // bits 2-3 in byte 7 distinguish NES 2.0 (0b10) from iNES 1.0.
        // Other nonzero combinations are not supported by this decoder.
        if ((bytes[7] & 0x0cu) != 0u)
            throw std::invalid_argument(
                "Only iNES 1.0 ROMs are supported; NES 2.0 requires wider bank/mapper types.");

        if (bytes[4] == 0u)
            throw std::invalid_argument(
                "iNES 1.0 cartridge must contain at least one 16 KiB PRG bank.");

        // Four-screen VRAM wiring is not part of the current MIRROR enum.
        if ((bytes[6] & 0x08u) != 0u)
            throw std::invalid_argument(
                "Four-screen iNES nametable mirroring is not implemented.");

        const auto mapper = static_cast<std::uint8_t>(
            (bytes[7] & 0xf0u) | (bytes[6] >> 4u));
        switch (mapper)
        {
            case 0u: case 1u: case 2u: case 3u:
            case 4u: case 66u: break;
            default:
                throw std::invalid_argument(
                    "iNES cartridge uses an unsupported mapper ID.");
        }

        INES1Header result;
        result.Mapper = mapper;
        result.PRGBanks = bytes[4];
        result.CHRBanks = bytes[5];
        result.TrainerPresent = (bytes[6] & 0x04u) != 0u;
        result.VerticalMirroring = (bytes[6] & 0x01u) != 0u;
        result.PRGBytes = static_cast<std::size_t>(result.PRGBanks) * 16u * 1024u;
        result.CHRFileBytes = static_cast<std::size_t>(result.CHRBanks) * 8u * 1024u;
        result.CHRStorageBytes = result.CHRFileBytes == 0u ?
            8u * 1024u : result.CHRFileBytes;
        return result;
    }

    [[nodiscard]] inline INES1RomPayload ReadINES1Rom(std::istream& input)
    {
        std::array<std::uint8_t, 16u> raw{};
        if (!input.read(reinterpret_cast<char*>(raw.data()),
            static_cast<std::streamsize>(raw.size())))
            throw std::invalid_argument("iNES ROM header is truncated.");

        INES1RomPayload image;
        image.Header = ParseINES1Header(raw); // before allocating ROM banks

        if (image.Header.TrainerPresent)
        {
            std::array<char, 512u> trainer{};
            if (!input.read(trainer.data(),
                static_cast<std::streamsize>(trainer.size())))
                throw std::invalid_argument("iNES ROM trainer is truncated.");
        }

        image.PRG.resize(image.Header.PRGBytes);
        if (!input.read(reinterpret_cast<char*>(image.PRG.data()),
            static_cast<std::streamsize>(image.PRG.size())))
            throw std::invalid_argument("iNES PRG ROM payload is truncated.");

        // CHR bank count 0 means emulator-owned RAM filled with zeroes, not
        // 8192 bytes serialized after PRG. Reading there would reject valid
        // CHR-RAM cartridges and misalign any subsequent ROM data.
        image.CHR.resize(image.Header.CHRStorageBytes);
        if (image.Header.CHRFileBytes != 0u &&
            !input.read(reinterpret_cast<char*>(image.CHR.data()),
                static_cast<std::streamsize>(image.Header.CHRFileBytes)))
            throw std::invalid_argument("iNES CHR ROM payload is truncated.");

        return image;
    }
}
