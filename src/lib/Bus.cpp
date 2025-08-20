#include "../../include/Bus.h"
#include "../../include/Cartridge.h"
#include "../../include/Ram.h"

Bus::Bus(std::shared_ptr<Cartridge> cartridge, std::shared_ptr<RAM> ram) : cartridge(cartridge), ram(ram)
{
}

uint8_t Bus::read8(uint16_t address)
{
  if (address < 0x8000)
  {
    // Cartridge
    return cartridge->read(address);
  }
  else if (address < 0xA000)
  {
    // PPU VRAM
  }
  else if (address < 0xC000)
  {
    // External RAM
  }
  else if (address < 0xE000)
  {
    return ram->readWRAM(address);
  }
  else if (address < 0xFE00)
  {
    // echo RAM
    return 0;
  }
  else if (address < 0xFEA0)
  {
    // Object Attribute Memory (OAM)
  }
  else if (address < 0xFF00)
  {
    // Reserved
    return 0;
  }
  else if (address < 0xFF80)
  {
    // I/O Registers
  }
  else if (address == 0xFFFF)
  {
    // Interrupt Enable Register
  }
  return ram->readHRAM(address);
}

void Bus::write8(uint16_t address, uint8_t value)
{
  if (address < 0x8000)
  {
    // Cartridge
  }
  else if (address < 0xA000)
  {
    // PPU VRAM
  }
  else if (address < 0xC000)
  {
    // External RAM
  }
  else if (address < 0xE000)
  {
    ram->writeWRAM(address, value);
  }
  else if (address < 0xFE00)
  {
  }
  else if (address < 0xFEA0)
  {
    // Object Attribute Memory (OAM)
  }
  else if (address < 0xFF00)
  {
  }
  else if (address < 0xFF80)
  {
    // I/O Registers
  }
  else if (address == 0xFFFF)
  {
    // Interrupt Enable Register
  }
  else
  {
    ram->writeHRAM(address, value);
  }
}