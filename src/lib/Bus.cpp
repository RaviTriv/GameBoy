#include "Bus.h"
#include "Cartridge.h"
#include "Common.h"
#include "Dma.h"
#include "Io.h"
#include "Logger.h"
#include "Ppu.h"
#include "Ram.h"

Bus::Bus(Cartridge &cartridge, InterruptRegs interruptRegs, DMA &dma, IO &io,
         PPU &ppu, RAM &ram)
    : cartridge(cartridge), interruptRegs(interruptRegs), dma(dma), io(io),
      ppu(ppu), ram(ram) {}

uint8_t Bus::read8(uint16_t address) {
  switch (PAGE_TABLE[address >> 8]) {
  case RegionType::CartROM:
    return cartridge.read(address);
  case RegionType::VRAM:
    return ppu.vramRead(address);
  case RegionType::CartRAM:
    return cartridge.read(address);
  case RegionType::WRAM:
    return ram.readWRAM(address);
  case RegionType::EchoRAM:
    return 0;
  case RegionType::MixedFE:
    return readPageFE(address);
  case RegionType::MixedFF:
    return readPageFF(address);
  }
  return INVALID_READ_VALUE;
}

uint8_t Bus::readPageFE(uint16_t address) {
  if (address <= OAM_END) {
    if (dma.isTransferring()) {
      return 0xFF;
    }
    return ppu.oamRead(address);
  }
  return 0;
}

uint8_t Bus::readPageFF(uint16_t address) {
  if (address <= IO_REGISTERS_END) {
    return io.read(address);
  }
  if (address == IE_REGISTER) {
    return interruptRegs.ie;
  }
  return ram.readHRAM(address);
}

uint16_t Bus::read16(uint16_t address) {
  uint8_t low = read8(address);
  uint8_t high = read8(address + 1);
  return low | (high << BYTE_BITS);
}

void Bus::write8(uint16_t address, uint8_t value) {
  switch (PAGE_TABLE[address >> 8]) {
  case RegionType::CartROM:
    cartridge.write(address, value);
    return;
  case RegionType::VRAM:
    ppu.vramWrite(address, value);
    return;
  case RegionType::CartRAM:
    cartridge.write(address, value);
    return;
  case RegionType::WRAM:
    ram.writeWRAM(address, value);
    return;
  case RegionType::EchoRAM:
    return;
  case RegionType::MixedFE:
    writePageFE(address, value);
    return;
  case RegionType::MixedFF:
    writePageFF(address, value);
    return;
  }
}

void Bus::writePageFE(uint16_t address, uint8_t value) {
  if (address <= OAM_END) {
    if (dma.isTransferring()) {
      return;
    }
    ppu.oamWrite(address, value);
  }
}

void Bus::writePageFF(uint16_t address, uint8_t value) {
  if (address <= IO_REGISTERS_END) {
    io.write(address, value);
  } else if (address == IE_REGISTER) {
    interruptRegs.ie = value;
  } else {
    ram.writeHRAM(address, value);
  }
}

void Bus::write16(uint16_t address, uint16_t value) {
  write8(address + 1, (value >> BYTE_BITS) & BYTE_MASK);
  write8(address, value & BYTE_MASK);
}