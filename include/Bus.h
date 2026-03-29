#pragma once

#include "IMemAccess.h"
#include "InterruptRegs.h"
#include <cstdint>
#include <functional>

class Cartridge;
class IO;
class PPU;
class RAM;
class Bus : public IMemAccess {
public:
  Bus(Cartridge &cartridge, InterruptRegs interruptRegs,
      std::function<bool()> isDmaTransferring, IO &io,
      PPU &ppu, RAM &ram);
  uint8_t read8(uint16_t address) override;
  uint16_t read16(uint16_t address) override;
  void write8(uint16_t address, uint8_t value) override;
  void write16(uint16_t address, uint16_t value) override;

private:
  Cartridge &cartridge;
  InterruptRegs interruptRegs;
  std::function<bool()> isDmaTransferring;
  IO &io;
  PPU &ppu;
  RAM &ram;
};