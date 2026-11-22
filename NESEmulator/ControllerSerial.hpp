#pragma once

// CPU-visible NES controller register shift behavior. The host latches eight
// button bits into Bus::controller_state on $4016/$4017 writes. Subsequent
// CPU reads return the current most-significant bit and consume one button.
//
// Debugger/disassembler reads request bReadOnly=true. They must be able to
// inspect the same bit WITHOUT consuming it; otherwise merely opening the
// memory debugger changes gameplay input.
//
// This helper is deliberately C++23 standard-library only. It can be unit
// tested without a ROM, SDL/OpenGL, mapper, pixel engine or hardware clock.
#include <cstdint>

namespace kairo::nes::detail
{
    // A write to CPU $4016 snapshots BOTH player button bytes in this
    // emulator's simplified latch model. CPU $4017 writes belong to the
    // APU frame counter; they must not be mistaken for a player-2 latch.
    // Real NES strobe-high and falling-edge timing is still future work.
    inline void LatchControllerPair(
        std::uint8_t (&shiftRegisters)[2],
        const std::uint8_t (&liveButtons)[2]) noexcept
    {
        shiftRegisters[0] = liveButtons[0];
        shiftRegisters[1] = liveButtons[1];
    }

    [[nodiscard]] inline std::uint8_t ReadControllerSerialBit(
        std::uint8_t& latchedButtons, bool readOnly) noexcept
    {
        const std::uint8_t bit =
            (latchedButtons & static_cast<std::uint8_t>(0x80u)) != 0u ? 1u : 0u;
        if (!readOnly)
            latchedButtons = static_cast<std::uint8_t>(latchedButtons << 1u);
        return bit;
    }
}
