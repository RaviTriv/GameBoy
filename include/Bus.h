#pragma once

#include "IMemAccess.h"
#include <cstdint>
#include <memory>

class Cartridge;
class CPU;
class DMA;
class IO;
class PPU;
class RAM;
class Bus : public IMemAccess {
public:
  Bus(std::shared_ptr<Cartridge> cartridge, std::shared_ptr<CPU> cpu,
      std::shared_ptr<DMA> dma, std::shared_ptr<IO> io,
      std::shared_ptr<PPU> ppu, std::shared_ptr<RAM> ram);
  uint8_t read8(uint16_t address) override;
  uint16_t read16(uint16_t address) override;
  void write8(uint16_t address, uint8_t value) override;
  void write16(uint16_t address, uint16_t value) override;
  void setCpu(std::shared_ptr<CPU> cpu);

private:
  std::shared_ptr<Cartridge> cartridge;
  std::shared_ptr<CPU> cpu;
  std::shared_ptr<DMA> dma;
  std::shared_ptr<IO> io;
  std::shared_ptr<PPU> ppu;
  std::shared_ptr<RAM> ram;
};