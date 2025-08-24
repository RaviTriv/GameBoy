#pragma once

#include <cstdint>
#include <memory>

class Cartridge;
class IO;
class PPU;
class RAM;
class Bus
{
public:
  Bus(std::shared_ptr<Cartridge> cartridge, std::shared_ptr<IO> io, std::shared_ptr<PPU> ppu, std::shared_ptr<RAM> ram);
  uint8_t read8(uint16_t address);
  uint16_t read16(uint16_t address);
  void write8(uint16_t address, uint8_t value);
  void write16(uint16_t address, uint16_t value);

private:
  std::shared_ptr<Cartridge> cartridge;
  std::shared_ptr<IO> io;
  std::shared_ptr<PPU> ppu;
  std::shared_ptr<RAM> ram;
};