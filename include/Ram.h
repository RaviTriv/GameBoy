#pragma once

#include <array>
#include <cstdint>

#include "Common.h"
#include "Serializable.h"

class RAM {
 public:
  struct State {
    std::array<uint8_t, 0x2000> wram = {};
    std::array<uint8_t, 0x80> hram = {};
  };

  [[nodiscard]] uint8_t readWRAM(uint16_t address) const
      pre(address >= WRAM_START && address <= WRAM_END);
  void writeWRAM(uint16_t address, uint8_t value)
      pre(address >= WRAM_START && address <= WRAM_END);
  [[nodiscard]] uint8_t readHRAM(uint16_t address) const
      pre(address >= HRAM_START && address <= HRAM_END);
  void writeHRAM(uint16_t address, uint8_t value)
      pre(address >= HRAM_START && address <= HRAM_END);
  void save(std::ostream& out) const;
  void load(std::istream& in);

 private:
  State state;
  static constexpr uint16_t WRAM_BASE = 0xC000;
  static constexpr uint16_t HRAM_BASE = 0xFF80;
};