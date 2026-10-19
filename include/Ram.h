#pragma once

#include <array>
#include <cstdint>

#include "Serializable.h"

class RAM : public Serializable {
 public:
  struct State {
    std::array<uint8_t, 0x2000> wram = {};
    std::array<uint8_t, 0x80> hram = {};
  };

  [[nodiscard]] uint8_t readWRAM(uint16_t address) const;
  void writeWRAM(uint16_t address, uint8_t value);
  [[nodiscard]] uint8_t readHRAM(uint16_t address) const;
  void writeHRAM(uint16_t address, uint8_t value);
  void save(std::ostream& out) const override;
  void load(std::istream& in) override;

 private:
  State state;
  static constexpr uint16_t WRAM_BASE = 0xC000;
  static constexpr uint16_t HRAM_BASE = 0xFF80;
};