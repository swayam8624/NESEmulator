// CPU-only regression for Bus::$4016/$4017 controller serial semantics.
// Specifically verifies that debugger read-only access does NOT consume
// input bits; no rendering, ROM images or third-party libraries are needed.
#include "../ControllerSerial.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

namespace
{
    void Require(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }
}

int main()
{
    try
    {
        using kairo::nes::detail::ReadControllerSerialBit;

        std::uint8_t player1 = 0b10110000u;
        std::uint8_t player2 = 0b01001010u;

        // A debugger may inspect $4016 several times between CPU clocks:
        // each peek returns the SAME current button, with no state mutation.
        for (int i = 0; i < 12; ++i)
        {
            Require(ReadControllerSerialBit(player1, true) == 1u &&
                player1 == 0b10110000u,
                "Read-only controller peek unexpectedly shifted input bits");
        }

        const std::array<std::uint8_t, 8> expectedP1{
            1u, 0u, 1u, 1u, 0u, 0u, 0u, 0u
        };
        const std::array<std::uint8_t, 8> expectedP2{
            0u, 1u, 0u, 0u, 1u, 0u, 1u, 0u
        };
        for (std::size_t i = 0u; i < expectedP1.size(); ++i)
        {
            // The normal CPU read consumes exactly one serial bit.
            Require(ReadControllerSerialBit(player1, false) == expectedP1[i],
                "Player 1 serial bit order changed");
            // The next debug peek must not consume a different bit.
            const auto beforePeek = player1;
            (void)ReadControllerSerialBit(player1, true);
            Require(player1 == beforePeek,
                "Interleaved debugger read altered controller state");

            Require(ReadControllerSerialBit(player2, false) == expectedP2[i],
                "Player 2 button stream or isolation changed");
        }
        Require(player1 == 0u && player2 == 0u,
            "Eight serial reads did not exhaust the current shift registers");

        std::uint8_t zero = 0u;
        Require(ReadControllerSerialBit(zero, true) == 0u &&
            ReadControllerSerialBit(zero, false) == 0u && zero == 0u,
            "All-zero controller latch behaved unexpectedly");

        // This emulator currently produces zero after eight consumed bits;
        // actual NES controller post-eight-bit behavior needs its own hardware
        // compatibility test before changing that separate policy.
        std::puts("NES controller serial read-only regression tests passed");
        return 0;
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "NES controller regression: %s\n", e.what());
        return 1;
    }
}
