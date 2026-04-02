#include "Ppu.h"
#include "InterruptSink.h"
#include "Lcd.h"
#include "Logger.h"

PPU::PPU(InterruptSink &interruptSink) : interruptSink(interruptSink), pipeline(this)
{
}

void PPU::init()
{
  lcd->setLcdMode(LCD::MODE::OAM);
}

const std::array<uint32_t, PPU::BUFFER_SIZE> &PPU::getVideoBuffer() const
{
  return videoBuffers[readBufferIndex.load(std::memory_order_acquire)];
}

std::array<uint32_t, PPU::BUFFER_SIZE> &PPU::getWriteBuffer()
{
  return videoBuffers[1 - readBufferIndex.load(std::memory_order_relaxed)];
}

void PPU::tick()
{
  state.lineTicks++;
  switch (lcd->getLcdMode())
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

uint8_t PPU::oamRead(uint16_t address) const
{
  if (address >= OAM_START_ADDR)
  {
    address -= OAM_START_ADDR;
  }
  const uint8_t *p = reinterpret_cast<const uint8_t *>(state.oamRam.data());
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
  if (pipeline.isWindowVisible() && lcd->state.ly >= lcd->state.windowY && lcd->state.ly < lcd->state.windowY + YRES)
  {
    state.windowLine++;
  }

  lcd->state.ly++;

  if (lcd->state.ly == lcd->state.lyCompare)
  {
    lcd->setLycFlag(1);

    if (lcd->isLcdStatIntEnabled(static_cast<uint8_t>(LCD::LCDS_SRC::S_LYC)))
    {
      interruptSink.requestInterrupt(InterruptType::LCD_STAT);
    }
  }
  else
  {
    lcd->setLycFlag(0);
  }
}

void PPU::loadLineSprites()
{
  int curY = lcd->state.ly;

  uint8_t spriteHeight = lcd->getObjHeight();

  for (int i = 0; i < 40; i++)
  {
    OAM_ENTRY entry = state.oamRam[i];

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
      state.lineSpritesCount++;
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
    lcd->setLcdMode(LCD::MODE::DRAWING);

    pipeline.oamReset();
  }
}

void PPU::drawingMode()
{
  pipeline.process();

  if (pipeline.getPushedCount() >= XRES)
  {
    pipeline.reset();
    lcd->setLcdMode(LCD::MODE::HBLANK);

    if (lcd->state.lcds & (static_cast<uint8_t>(LCD::LCDS_SRC::S_HBLANK)))
    {
      interruptSink.requestInterrupt(InterruptType::LCD_STAT);
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
      lcd->setLcdMode(LCD::MODE::VBLANK);

      interruptSink.requestInterrupt(InterruptType::VBLANK);

      if (lcd->isLcdStatIntEnabled(static_cast<uint8_t>(LCD::LCDS_SRC::S_VBLANK)))
      {
        interruptSink.requestInterrupt(InterruptType::LCD_STAT);
      }

      currentFrame.fetch_add(1, std::memory_order_relaxed);
      readBufferIndex.store(
          1 - readBufferIndex.load(std::memory_order_relaxed),
          std::memory_order_release);
      if (!fastForward.load(std::memory_order_relaxed))
      {
        uint32_t end = getTicksFn();
        uint32_t frameTime = end - prevFrameTime;
        if (frameTime < targetFrameTime)
        {
          delayFn((targetFrameTime - frameTime));
        }
        if (end - startTimer >= 1000)
        {
          startTimer = end;
          frameCount = 0;
        }

        frameCount++;
        prevFrameTime = getTicksFn();
      }
    }
    else
    {
      lcd->setLcdMode(LCD::MODE::OAM);
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
      lcd->setLcdMode(LCD::MODE::OAM);
      lcd->state.ly = 0;
      state.windowLine = 0;
    }
    state.lineTicks = 0;
  }
}

/*
<-----PPU-SM-END----->
*/

uint32_t PPU::getCurrentFrame() const
{
  return currentFrame.load(std::memory_order_relaxed);
}

void PPU::setCurrentFrame(uint32_t frame)
{
  currentFrame.store(frame, std::memory_order_relaxed);
}

void PPU::setVideoBuffer(const std::array<uint32_t, BUFFER_SIZE> &buffer)
{
  videoBuffers[0] = buffer;
  videoBuffers[1] = buffer;
}

PPU::State PPU::getState() const
{
  return state;
}

void PPU::setState(const State &state)
{
  this->state = state;
};

Pipeline::State PPU::getPipelineState() const
{
  return pipeline.state;
}

void PPU::setPipelineState(const Pipeline::State &state)
{
  pipeline.state = state;
};

void PPU::setFastForward(bool ff)
{
  fastForward.store(ff, std::memory_order_relaxed);
}

bool PPU::isFastForward() const
{
  return fastForward.load(std::memory_order_relaxed);
}