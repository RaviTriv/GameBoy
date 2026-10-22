#include "Bus.h"

#include <cstdint>

#include "Cartridge.h"
#include "Common.h"
#include "Dma.h"
#include "Interrupts.h"
#include "Io.h"
#include "Ppu.h"
#include "Ram.h"

Bus::Bus(Cartridge& cartridge, Interrupts& interrupts, DMA& dma, IO& io,
         PPU& ppu, RAM& ram)
    : cartridge_(cartridge),
      interrupts_(interrupts),
      dma_(dma),
      io_(io),
      ppu_(ppu),
      ram_(ram) {}

uint8_t Bus::read8(uint16_t address) {
  switch (PAGE_TABLE[address >> 8]) {
    case RegionType::CartROM:
      return cartridge_.read(address);
    case RegionType::VRAM:
      if (ppu_.isVramBlocked()) {
        return INVALID_READ_VALUE;
      }
      return ppu_.vramRead(address);
    case RegionType::CartRAM:
      return cartridge_.read(address);
    case RegionType::WRAM:
      return ram_.readWRAM(address);
    case RegionType::EchoRAM:
      return ram_.readWRAM(address - 0x2000);
    case RegionType::MixedFE:
      return readPageFE(address);
    case RegionType::MixedFF:
      return readPageFF(address);
  }
  return INVALID_READ_VALUE;
}

uint8_t Bus::readPageFE(uint16_t address) {
  if (address <= OAM_END) {
    if (dma_.isTransferring() || ppu_.isOamBlocked()) {
      return 0xFF;
    }
    return ppu_.oamRead(address);
  }
  return 0;
}

uint8_t Bus::readPageFF(uint16_t address) {
  if (address <= IO_REGISTERS_END) {
    return io_.read(address);
  }
  if (address == IE_REGISTER) {
    return interrupts_.ie();
  }
  return ram_.readHRAM(address);
}

void Bus::write8(uint16_t address, uint8_t value) {
  switch (PAGE_TABLE[address >> 8]) {
    case RegionType::CartROM:
      cartridge_.write(address, value);
      return;
    case RegionType::VRAM:
      ppu_.vramWrite(address, value);
      return;
    case RegionType::CartRAM:
      cartridge_.write(address, value);
      return;
    case RegionType::WRAM:
      ram_.writeWRAM(address, value);
      return;
    case RegionType::EchoRAM:
      ram_.writeWRAM(address - 0x2000, value);
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
    if (dma_.isTransferring()) {
      return;
    }
    ppu_.oamWrite(address, value);
  }
}

void Bus::writePageFF(uint16_t address, uint8_t value) {
  if (address <= IO_REGISTERS_END) {
    io_.write(address, value);
  } else if (address == IE_REGISTER) {
    interrupts_.setIe(value);
  } else {
    ram_.writeHRAM(address, value);
  }
}
