#include "../../../include/Ppu.h"
#include "../../../include/Lcd.h"
#include "../../../include/Logger.h"

PPU::PPU(std::shared_ptr<LCD> lcd) : lcd(lcd)
{
}

void PPU::tick()
{
  state.lineTicks++;
  switch (lcd->state.ppuMode)
  {
  case LCD::MODE::OAM:
    Logger::GetLogger()->info("OAM");
    break;
  case LCD::MODE::DRAWING:
    Logger::GetLogger()->info("VRAM");
    break;
  case LCD::MODE::VBLANK:
    Logger::GetLogger()->info("VBLANK");
    break;
  case LCD::MODE::HBLANK:
    Logger::GetLogger()->info("HBLANK");
    break;
  default:
    Logger::GetLogger()->error("Unknown PPU mode: {}", static_cast<int>(lcd->state.ppuMode));
    break;
  }
}

void PPU::oamWrite(uint16_t address, uint8_t value)
{
  if (address >= OAM_START_ADDR)
  {
    address -= OAM_START_ADDR;
  }
  uint8_t *p = reinterpret_cast<uint8_t *>(state.oamRam.data());
  p[address] = value;
}

uint8_t PPU::oamRead(uint16_t address)
{
  if (address >= OAM_START_ADDR)
  {
    address -= OAM_START_ADDR;
  }
  uint8_t *p = reinterpret_cast<uint8_t *>(state.oamRam.data());
  return p[address];
}

void PPU::vramWrite(uint16_t address, uint8_t value)
{
  state.vram[address - VRAM_START_ADDR] = value;
}

uint8_t PPU::vramRead(uint16_t address) const
{
  return state.vram[address - VRAM_START_ADDR];
}