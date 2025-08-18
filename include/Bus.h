#pragma once

#include <cstdint>
#include <memory>

class Cartridge;
class Bus
{
public:
  Bus(std::shared_ptr<Cartridge> cartridge);
  uint8_t read(uint16_t address);
  uint16_t read16(uint16_t address);
  void write(uint16_t address, uint8_t value);
  void write16(uint16_t address, uint16_t value);

private:
  std::shared_ptr<Cartridge> cartridge;
};