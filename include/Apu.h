#pragma once

#include "./Channel.h"

#include <array>
#include <cstdint>

class APU
{
  struct Registers
  {
    uint8_t NR52;
    uint8_t NR51;
    uint8_t NR50;
  };

  struct State
  {
    Registers registers;
    std::array<uint8_t, 16> wavePattern;
    bool enabled;
    uint32_t sampleRate;
    uint32_t cycleCounter;
  };

public:
  void write(uint16_t address, uint8_t value);
  uint8_t read(uint16_t address);

private:
  State state;
};