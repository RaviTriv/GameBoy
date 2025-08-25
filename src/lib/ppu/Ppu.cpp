#include "../../../include/Ppu.h"
#include "../../../include/Cpu.h"
#include "../../../include/Lcd.h"
#include "../../../include/Logger.h"

PPU::PPU(std::shared_ptr<CPU> cpu, std::shared_ptr<LCD> lcd) : cpu(cpu), lcd(lcd), pipeline(this)
{
}

void PPU::setCpu(std::shared_ptr<CPU> cpu) { this->cpu = cpu; }

void PPU::tick()
{
  state.lineTicks++;
  switch (lcd->state.lcdsBits.ppuMode)
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
    Logger::GetLogger()->error("Unknown PPU mode: {}", static_cast<int>(lcd->state.lcdsBits.ppuMode));
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
    lcd->state.lcdsBits.lycFlag = 1;
    if (lcd->isLcdStatIntEnabled(static_cast<uint8_t>(LCD::LCDS_SRC::S_LYC)))
    {
      cpu->requestInterrupt(InterruptType::LCD_STAT);
    }
  }
  else
  {
    lcd->state.lcdsBits.lycFlag = 0;
  }
}

void PPU::oamMode()
{
  if (state.lineTicks == 1)
  {
    state.currentLineSprites.clear();

    // TODO: Load sprites for the line
  }

  if (state.lineTicks >= 80)
  {
    lcd->state.lcdsBits.ppuMode = LCD::MODE::DRAWING;

    // TODO: Update State
  }
}

void PPU::drawingMode()
{
  // TODO: Process pipeline

  // Call HBLANK after x > XRES
  lcd->state.lcdsBits.ppuMode = LCD::MODE::HBLANK;
}

void PPU::hBlankMode()
{

  if (state.lineTicks >= TICKS_PER_LINE)
  {
    incrementLY();

    if (lcd->state.ly >= YRES)
    {
      lcd->state.lcdsBits.ppuMode = LCD::MODE::VBLANK;

      cpu->requestInterrupt(InterruptType::VBLANK);

      if (lcd->isLcdStatIntEnabled(static_cast<uint8_t>(LCD::LCDS_SRC::S_VBLANK)))
      {
        cpu->requestInterrupt(InterruptType::LCD_STAT);
      }

      // TODO: Add in Delay and Leverage this for Fast Foward Feature
    }
    else
    {
      lcd->state.lcdsBits.ppuMode = LCD::MODE::OAM;
    }
  }
}

void PPU::vBlankMode()
{
  if (state.lineTicks >= TICKS_PER_LINE)
  {
    incrementLY();

    if (lcd->state.ly >= LINES_PER_FRAME)
    {
      lcd->state.lcdsBits.ppuMode = LCD::MODE::OAM;
      lcd->state.ly = 0;
      state.windowLine = 0;
    }
    state.lineTicks = 0;
  }
}

/*
<-----PPU-SM-END----->
*/