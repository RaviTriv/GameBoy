#include "../../../include/Ppu.h"
#include "../../../include/Cpu.h"
#include "../../../include/Lcd.h"
#include "../../../include/Logger.h"

PPU::PPU(std::shared_ptr<Bus> bus, std::shared_ptr<CPU> cpu, std::shared_ptr<LCD> lcd) : bus(bus), cpu(cpu), lcd(lcd), pipeline(this)
{
  std::fill(state.videoBuffer.begin(), state.videoBuffer.end(), 0xFFFFFFFF);
}

void PPU::init()
{
  lcd->state.lcdsBits.ppuMode = LCD::MODE::OAM;
}

void PPU::setCpu(std::shared_ptr<CPU> cpu) { this->cpu = cpu; }

void PPU::setBus(std::shared_ptr<Bus> bus) { this->bus = bus; }

const std::array<uint32_t, PPU::BUFFER_SIZE> &PPU::getVideoBuffer() const
{
  return state.videoBuffer;
}

void PPU::tick()
{
  state.lineTicks++;
  switch (lcd->state.lcdsBits.ppuMode)
  {
  case LCD::MODE::OAM:
    oamMode();
    break;
  case LCD::MODE::DRAWING:
    drawingMode();
    break;
  case LCD::MODE::HBLANK:
    hBlankMode();
    break;
  case LCD::MODE::VBLANK:
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
  if (pipeline.windowVisible() && lcd->state.ly >= lcd->state.windowY && lcd->state.ly < lcd->state.windowY + YRES)
  {
    state.windowLine++;
  }

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

void PPU::loadLineSprites()
{
  int curY = lcd->state.ly;

  uint8_t spriteHeight = lcd->state.lcdcBits.objSize ? 16 : 8;

  pipeline.clearFetchedEntries();

  for (int i = 0; i < 40; i++)
  {
    OAM_ENTRY &entry = state.oamRam[i];

    if (!entry.x)
    {
      continue;
    }

    if (state.lineSpritesCount >= 10)
    {
      break;
    }

    if (entry.y <= curY + 16 && entry.y + spriteHeight > curY + 16)
    {
      auto iter = state.currentLineSprites.begin();
      while (iter != state.currentLineSprites.end() && iter->x <= entry.x)
      {
        ++iter;
      }

      state.currentLineSprites.insert(iter, {entry});
    }
  }
}

void PPU::oamMode()
{
  if (state.lineTicks == 1)
  {
    state.currentLineSprites.clear();
    state.lineSpritesCount = 0;
    loadLineSprites();
  }

  if (state.lineTicks >= 80)
  {
    lcd->state.lcdsBits.ppuMode = LCD::MODE::DRAWING;

    pipeline.oamReset();
  }
}

void PPU::drawingMode()
{
  pipeline.process();

  if (pipeline.getPushedCount() >= XRES)
  {
    pipeline.reset();
    lcd->state.lcdsBits.ppuMode = LCD::MODE::HBLANK;

    if (lcd->state.lcds & (static_cast<uint8_t>(LCD::LCDS_SRC::S_HBLANK)))
    {
      cpu->requestInterrupt(InterruptType::LCD_STAT);
    }
  }
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
    state.lineTicks = 0;
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