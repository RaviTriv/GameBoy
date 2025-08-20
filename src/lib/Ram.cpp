#include "../../include/Ram.h"
#include "../../include/Logger.h"

uint8_t RAM::readWRAM(uint16_t address) const
{
  if (address < WRAM_BASE || address >= WRAM_BASE + state.wram.size())
  {
    Logger::GetLogger()->error("WRAM read out of bounds: 0x{:04X}", address);
    return 0xFF;
  }
  size_t offset = address - WRAM_BASE;
  return state.wram[offset];
}

void RAM::writeWRAM(uint16_t address, uint8_t value)
{
  if (address < WRAM_BASE || address >= WRAM_BASE + state.wram.size())
  {
    Logger::GetLogger()->error("WRAM write out of bounds: 0x{:04X}", address);
    return;
  }
  size_t offset = address - WRAM_BASE;
  state.wram[offset] = value;
}

uint8_t RAM::readHRAM(uint16_t address) const
{
  if (address < HRAM_BASE || address >= HRAM_BASE + state.hram.size())
  {
    Logger::GetLogger()->error("HRAM read out of bounds: 0x{:04X}", address);
    return 0xFF;
  }
  size_t offset = address - HRAM_BASE;
  return state.hram[offset];
}

void RAM::writeHRAM(uint16_t address, uint8_t value)
{
  if (address < HRAM_BASE || address >= HRAM_BASE + state.hram.size())
  {
    Logger::GetLogger()->error("HRAM write out of bounds: 0x{:04X}", address);
    return;
  }
  size_t offset = address - HRAM_BASE;
  state.hram[offset] = value;
}
