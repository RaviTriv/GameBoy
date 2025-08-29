#include "../../include/Bus.h"
#include "../../include/Cartridge.h"
#include "../../include/Cpu.h"
#include "../../include/Dma.h"
#include "../../include/Io.h"
#include "../../include/Ppu.h"
#include "../../include/Ram.h"
#include "../../include/Logger.h"

Bus::Bus(std::shared_ptr<Cartridge> cartridge, std::shared_ptr<CPU> cpu, std::shared_ptr<DMA> dma, std::shared_ptr<IO> io, std::shared_ptr<PPU> ppu, std::shared_ptr<RAM> ram) : cartridge(cartridge), cpu(cpu), dma(dma), io(io), ppu(ppu), ram(ram)
{
}

void Bus::setCpu(std::shared_ptr<CPU> cpu) { this->cpu = cpu; }

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
    return ppu->vramRead(address);
  }
  else if (address < 0xC000)
  {
    // External RAM
    return cartridge->read(address);
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
    if (dma->isTransferring())
    {
      return 0xFF;
    }
    return ppu->oamRead(address);
  }
  else if (address < 0xFF00)
  {
    // Reserved
    return 0;
  }
  else if (address < 0xFF80)
  {
    // I/O Registers
    return io->read(address);
  }
  else if (address == 0xFFFF)
  {
    // Interrupt Enable Register
    return cpu->getInterruptEnable();
  }
  return ram->readHRAM(address);
}

uint16_t Bus::read16(uint16_t address)
{
  uint8_t low = read8(address);
  uint8_t high = read8(address + 1);
  return low | (high << 8);
}

void Bus::write8(uint16_t address, uint8_t value)
{
  if (address < 0x8000)
  {
    // Cartridge
    // TODO
  }
  else if (address < 0xA000)
  {
    // PPU VRAM
    ppu->vramWrite(address, value);
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
    if (dma->isTransferring())
    {
      return;
    }
    ppu->oamWrite(address, value);
  }
  else if (address < 0xFF00)
  {
  }
  else if (address < 0xFF80)
  {
    // I/O Registers
    io->write(address, value);
  }
  else if (address == 0xFFFF)
  {
    // Interrupt Enable Register
    cpu->setInterruptEnable(value);
  }
  else
  {
    ram->writeHRAM(address, value);
  }
}

void Bus::write16(uint16_t address, uint16_t value)
{
  write8(address + 1, (value >> 8) & 0xFF);
  write8(address, value & 0xFF);
}