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
    oamMode();
    break;
  case LCD::MODE::DRAWING:
    Logger::GetLogger()->info("DRAWING");
    drawingMode();
    break;
  case LCD::MODE::HBLANK:
    Logger::GetLogger()->info("HBLANK");
    hBlankMode();
    break;
  case LCD::MODE::VBLANK:
    Logger::GetLogger()->info("VBLANK");
    vBlankMode();
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

/*
<-----PPU-SM-START----->
*/

void PPU::incrementLY()
{
  lcd->state.ly++;
  if (lcd->state.ly == lcd->state.lyCompare)
  {
    Logger::GetLogger()->info("TRIGGER INTERRUPT HERE");
  }
  else
  {
    Logger::GetLogger()->info("SET FLAG");
  }
}

void PPU::oamMode()
{
  lcd->state.ppuMode = LCD::MODE::DRAWING;
}

void PPU::drawingMode()
{
  lcd->state.ppuMode = LCD::MODE::HBLANK;
}

void PPU::hBlankMode()
{
  lcd->state.ppuMode = LCD::MODE::VBLANK;
}

void PPU::vBlankMode()
{
  lcd->state.ppuMode = LCD::MODE::OAM;
}

/*
<-----PPU-SM-END----->
*/