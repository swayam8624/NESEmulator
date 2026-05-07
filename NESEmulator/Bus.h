//
// Created by Swayam Singal on 07/05/26.
//

#ifndef NESEMULATOR_BUS_H
#define NESEMULATOR_BUS_H

#pragma once
#include <cstdint>
#include <array>

#include "cpu6502.h"

class Bus
{
public:
    Bus();
    ~Bus();

public: // Devices on bus
    cpu6502 cpu;

    // Fake RAM for this part of the series
    std::array<uint8_t, 64 * 1024> ram;


public: // Bus Read & Write
    void write(uint16_t addr, uint8_t data);
    uint8_t read(uint16_t addr, bool bReadOnly = false);
};
