#include "../../../include/Ppu.h"
#include "../../../include/Cpu.h"
#include "../../../include/Lcd.h"
#include "../../../include/Ui.h"
#include "../../../include/Logger.h"

PPU::PPU(std::shared_ptr<Bus> bus, std::shared_ptr<CPU> cpu, std::shared_ptr<LCD> lcd, std::shared_ptr<UI> ui) : bus(bus), cpu(cpu), lcd(lcd), pipeline(this), ui(ui)
{
}

void PPU::setCpu(std::shared_ptr<CPU> cpu) { this->cpu = cpu; }

void PPU::setBus(std::shared_ptr<Bus> bus) { this->bus = bus; }

void PPU::setUi(std::shared_ptr<UI> ui) { this->ui = ui; }

void PPU::init()
{
  lcd->setLcdMode(MODE::OAM);
}

const std::array<uint32_t, PPU::BUFFER_SIZE> &PPU::getVideoBuffer() const
{
  return state.videoBuffer;
}

void PPU::tick()
{
  state.lineTicks++;
  switch (lcd->getLcdMode())
  {
  case MODE::OAM:
    oamMode();
    break;
  case MODE::DRAWING:
    drawingMode();
    break;
  case MODE::HBLANK:
    hBlankMode();
    break;
  case MODE::VBLANK:
    vBlankMode();
    break;
  default:
    Logger::GetLogger()->error("Unknown PPU mode: {}", lcd->getLcdMode());
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
  if (pipeline.windowVisible() && lcd->getLy() >= lcd->getWinY() &&
      lcd->getLy() < lcd->getWinY() + YRES)
  {
    state.windowLine++;
  }

  lcd->incrementLy();

  if (lcd->getLy() == lcd->getLyCompare())
  {
    lcd->setLycFlag(1);

    if (lcd->isLcdStatIntEnabled(static_cast<uint8_t>(LCDS_SRC::S_LYC)))
    {
      cpu->requestInterrupt(InterruptType::LCD_STAT);
    }
  }
  else
  {
    lcd->setLycFlag(0);
  }
}

void PPU::loadLineSprites()
{
  int curY = lcd->getLy();

  uint8_t spriteHeight = lcd->getObjHeight();

  for (int i = 0; i < 40; i++)
  {
    OAM_ENTRY entry = state.oamRam[i];

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
      auto idx = std::find_if(state.currentLineSprites.begin(),
                              state.currentLineSprites.end(),
                              [&entry](const OAM_ENTRY &e)
                              {
                                return e.x > entry.x;
                              });

      state.currentLineSprites.insert(idx, entry);
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
    lcd->setLcdMode(MODE::DRAWING);

    pipeline.oamReset();
  }
}

void PPU::drawingMode()
{
  pipeline.process();

  if (pipeline.getPushedCount() >= XRES)
  {
    pipeline.reset();
    lcd->setLcdMode(MODE::HBLANK);

    if (lcd->isLcdStatIntEnabled(static_cast<uint8_t>(LCDS_SRC::S_HBLANK)))
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

    if (lcd->getLy() >= YRES)
    {
      lcd->setLcdMode(MODE::VBLANK);

      cpu->requestInterrupt(InterruptType::VBLANK);

      if (lcd->isLcdStatIntEnabled(static_cast<uint8_t>(LCDS_SRC::S_VBLANK)))
      {
        cpu->requestInterrupt(InterruptType::LCD_STAT);
      }
      // TODO: Add in Delay and Leverage this for Fast Foward Feature
      state.currentFrame++;

      uint32_t end = ui->getTicks();
      uint32_t frameTime = end - prevFrameTime;
      if (frameTime < targetFrameTime)
      {
        ui->delay((targetFrameTime - frameTime));
      }
      if (end - startTimer >= 1000)
      {
        startTimer = end;
        frameCount = 0;
      }

      frameCount++;
      prevFrameTime = ui->getTicks();
    }
    else
    {
      lcd->setLcdMode(MODE::OAM);
    }

    state.lineTicks = 0;
  }
}

void PPU::vBlankMode()
{
  if (state.lineTicks >= TICKS_PER_LINE)
  {
    incrementLY();

    if (lcd->getLy() >= LINES_PER_FRAME)
    {
      lcd->setLcdMode(MODE::OAM);
      lcd->setLy(0);
      state.windowLine = 0;
    }
    state.lineTicks = 0;
  }
}

/*
<-----PPU-SM-END----->
*/

uint32_t PPU::getCurrentFrame()
{
  return state.currentFrame;
}