#pragma once

#include <array>
#include <cstdint>

class RAM
{
public:
  uint8_t readWRAM(uint16_t address) const;
  void writeWRAM(uint16_t address, uint8_t value);
  uint8_t readHRAM(uint16_t address) const;
  void writeHRAM(uint16_t address, uint8_t value);

private:
  struct State
  {
    std::array<uint8_t, 0x2000> wram = {};
    std::array<uint8_t, 0x80> hram = {};
  };
  State state;
  static constexpr uint16_t WRAM_BASE = 0xC000;
  static constexpr uint16_t HRAM_BASE = 0xFF80;
};