#include "../../include/Bus.h"
#include "../../include/Cartridge.h"
#include "../../include/Cpu.h"
#include "../../include/Dma.h"
#include "../../include/Io.h"
#include "../../include/Common.h"
#include "../../include/Ppu.h"
#include "../../include/Ram.h"
#include "../../include/Logger.h"

Bus::Bus(std::shared_ptr<Cartridge> cartridge, std::shared_ptr<CPU> cpu, std::shared_ptr<DMA> dma, std::shared_ptr<IO> io, std::shared_ptr<PPU> ppu, std::shared_ptr<RAM> ram) : cartridge(cartridge), cpu(cpu), dma(dma), io(io), ppu(ppu), ram(ram)
{
}

void Bus::setCpu(std::shared_ptr<CPU> cpu) { this->cpu = cpu; }

uint8_t Bus::read8(uint16_t address)
{
  if (address <= ROM_END)
  {
    // Cartridge
    return cartridge->read(address);
  }
  else if (address <= VRAM_END)
  {
    // PPU VRAM
    return ppu->vramRead(address);
  }
  else if (address <= CART_RAM_END)
  {
    // External RAM
    return cartridge->read(address);
  }
  else if (address <= WRAM_END)
  {
    return ram->readWRAM(address);
  }
  else if (address <= ECHO_RAM_END)
  {
    // echo RAM
    return 0;
  }
  else if (address <= OAM_END)
  {
    // Object Attribute Memory (OAM)
    if (dma->isTransferring())
    {
      return 0xFF;
    }
    return ppu->oamRead(address);
  }
  else if (address <= UNUSED_END)
  {
    // Reserved
    return 0;
  }
  else if (address <= IO_REGISTERS_END)
  {
    // I/O Registers
    return io->read(address);
  }
  else if (address == IE_REGISTER)
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
  return low | (high << BYTE_BITS);
}

void Bus::write8(uint16_t address, uint8_t value)
{
  if (address <= ROM_END)
  {
    // Cartridge
    cartridge->write(address, value);
  }
  else if (address <= VRAM_END)
  {
    // PPU VRAM
    ppu->vramWrite(address, value);
  }
  else if (address <= CART_RAM_END)
  {
    // External RAM
    cartridge->write(address, value);
  }
  else if (address <= WRAM_END)
  {
    ram->writeWRAM(address, value);
  }
  else if (address <= ECHO_RAM_END)
  {
  }
  else if (address <= OAM_END)
  {
    // Object Attribute Memory (OAM)
    if (dma->isTransferring())
    {
      return;
    }
    ppu->oamWrite(address, value);
  }
  else if (address <= UNUSED_END)
  {
  }
  else if (address <= IO_REGISTERS_END)
  {
    // I/O Registers
    io->write(address, value);
  }
  else if (address == IE_REGISTER)
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
  write8(address + 1, (value >> BYTE_BITS) & BYTE_MASK);
  write8(address, value & BYTE_MASK);
}