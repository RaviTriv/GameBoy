#pragma once

#include <array>
#include <cstdint>

class PPU
{
  struct State
  {
    std::array<uint8_t, 0x2000> vram;
  };

public:
  void tick();
private:
  State state;
};