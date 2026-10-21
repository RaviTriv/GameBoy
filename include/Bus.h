#pragma once

#include <array>
#include <cstdint>

#include "IMemAccess.h"
#include "Interrupts.h"

class Cartridge;
class DMA;
class IO;
class PPU;
class RAM;

enum class RegionType : uint8_t {
  CartROM,
  VRAM,
  CartRAM,
  WRAM,
  EchoRAM,
  MixedFE,
  MixedFF,
};

constexpr std::array<RegionType, 256> buildPageTable() {
  std::array<RegionType, 256> table{};
  for (std::size_t page = 0; page < 256; ++page) {
    auto base = static_cast<uint16_t>(page << 8);
    if (base <= 0x7F00) {
      table[page] = RegionType::CartROM;
    } else if (base <= 0x9F00) {
      table[page] = RegionType::VRAM;
    } else if (base <= 0xBF00) {
      table[page] = RegionType::CartRAM;
    } else if (base <= 0xDF00) {
      table[page] = RegionType::WRAM;
    } else if (base <= 0xFD00) {
      table[page] = RegionType::EchoRAM;
    } else if (page == 0xFE) {
      table[page] = RegionType::MixedFE;
    } else {
      table[page] = RegionType::MixedFF;
    }
  }
  return table;
}

inline constexpr auto PAGE_TABLE = buildPageTable();

class Bus final : public IMemAccess {
 public:
  Bus(Cartridge& cartridge, Interrupts& interrupts, DMA& dma, IO& io, PPU& ppu,
      RAM& ram);
  [[nodiscard]] uint8_t read8(uint16_t address) override;
  void write8(uint16_t address, uint8_t value) override;

 private:
  Cartridge& cartridge_;
  Interrupts& interrupts_;
  DMA& dma_;
  IO& io_;
  PPU& ppu_;
  RAM& ram_;

  [[nodiscard]] uint8_t readPageFE(uint16_t address);
  [[nodiscard]] uint8_t readPageFF(uint16_t address);
  void writePageFE(uint16_t address, uint8_t value);
  void writePageFF(uint16_t address, uint8_t value);
};
