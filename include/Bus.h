#pragma once

#include "IMemAccess.h"
#include "InterruptRegs.h"
#include <array>
#include <cstdint>

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
  for (int page = 0; page < 256; ++page) {
    uint16_t base = static_cast<uint16_t>(page << 8);
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

class Bus : public IMemAccess {
public:
  Bus(Cartridge &cartridge, InterruptRegs interruptRegs,
      DMA &dma, IO &io,
      PPU &ppu, RAM &ram);
  [[nodiscard]] uint8_t read8(uint16_t address) override;
  [[nodiscard]] uint16_t read16(uint16_t address) override;
  void write8(uint16_t address, uint8_t value) override;
  void write16(uint16_t address, uint16_t value) override;

private:
  Cartridge &cartridge;
  InterruptRegs interruptRegs;
  DMA &dma;
  IO &io;
  PPU &ppu;
  RAM &ram;

  [[nodiscard]] uint8_t readPageFE(uint16_t address);
  [[nodiscard]] uint8_t readPageFF(uint16_t address);
  void writePageFE(uint16_t address, uint8_t value);
  void writePageFF(uint16_t address, uint8_t value);
};
