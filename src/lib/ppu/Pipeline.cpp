#include "../../../include/Pipeline.h"
#include "../../../include/Bus.h"
#include "../../../include/Lcd.h"
#include "../../../include/Ppu.h"
#include "../../../include/Logger.h"

Pipeline::Pipeline(PPU *ppu) : ppu(ppu)
{
}

void Pipeline::process()
{
  state.mapY = calculateMapY();
  state.mapX = calculateMapX();
  state.tileY = calculateTileY();

  if ((ppu->state.lineTicks & 1) == 0)
  {
    fetch();
  }

  pushPixel();
}

void Pipeline::fetch()
{
  switch (state.fetchState)
  {
  case FETCH_STATE::TILE:
    fetchTile();
    break;
  case FETCH_STATE::DATA0:
    fetchData0();
    break;
  case FETCH_STATE::DATA1:
    fetchData1();
    break;
  case FETCH_STATE::IDLE:
    state.fetchState = FETCH_STATE::PUSH;
    break;
  case FETCH_STATE::PUSH:
    if (fifoAdd())
    {
      state.fetchState = FETCH_STATE::TILE;
    }
    break;
  default:
    throw std::runtime_error("Trying to process unknown fetch state");
    break;
  }
}

void Pipeline::fetchTile()
{
  state.entryCount = 0;

  if (ppu->lcd->isBgWindowEnabled())
  {
    state.bgwBuffer[0] = ppu->bus->read8(bgw0ReadAddress());

    if (ppu->lcd->getBgWindowDataArea() == 0x8800)
    {
      state.bgwBuffer[0] += 128;
    }
    loadWindowTile();
  }

  if (ppu->lcd->isObjEnabled() && !ppu->state.currentLineSprites.empty())
  {
    loadSpriteTile();
  }

  state.fetchState = FETCH_STATE::DATA0;
  state.fetchX += 8;
}

void Pipeline::fetchData0()
{
  state.bgwBuffer[1] = ppu->bus->read8(bgw1ReadAddress());
  loadSpriteData(0);
  state.fetchState = FETCH_STATE::DATA1;
}

void Pipeline::fetchData1()
{
  state.bgwBuffer[2] = ppu->bus->read8(bgw1ReadAddress() + 1);
  loadSpriteData(1);
  state.fetchState = FETCH_STATE::IDLE;
}

void Pipeline::pushPixel()
{
  if (state.fifoSize > 8)
  {
    uint32_t pixel = fifoPop();

    if (state.lineX >= (ppu->lcd->state.scrollX) % 8)
    {
      ppu->state.videoBuffer[bufferIndex()] = pixel;
      state.pushedCount++;
    }

    state.lineX++;
  }
}

void Pipeline::reset()
{
  while (!fifoIsEmpty())
  {
    fifoPop();
  }
}

uint8_t Pipeline::calculateMapY() const
{
  return ppu->lcd->state.ly + ppu->lcd->state.scrollY;
}

uint8_t Pipeline::calculateMapX() const
{
  return state.fetchX + ppu->lcd->state.scrollX;
}

uint8_t Pipeline::calculateTileY() const
{
  return ((ppu->lcd->state.ly + ppu->lcd->state.scrollY) % 8) * 2;
}

uint32_t Pipeline::bufferIndex() const
{
  return state.pushedCount + ppu->lcd->state.ly * PPU::XRES;
}

uint16_t Pipeline::bgw0ReadAddress() const
{
  return ppu->lcd->getBgMapArea() + (state.mapX / PIXEL_TILE_DIMENSION) + (((state.mapY / PIXEL_TILE_DIMENSION)) * BACKGROUND_MAP_DIMENSION);
}

uint16_t Pipeline::bgw1ReadAddress() const
{
  return ppu->lcd->getBgWindowDataArea() + (state.bgwBuffer[0] * 16) + state.tileY;
}

bool Pipeline::windowVisible() const
{
  return (ppu->lcd->isWindowEnabled()) && (ppu->lcd->state.windowX >= 0) && (ppu->lcd->state.windowX <= 166) && (ppu->lcd->state.windowY >= 0) && (ppu->lcd->state.windowY <= 144);
}

void Pipeline::loadWindowTile()
{
  if (!windowVisible())
  {
    return;
  }

  uint8_t windowY = ppu->lcd->state.windowY;

  if (state.fetchX + 7 >= ppu->lcd->state.windowX && state.fetchX + 7 < ppu->lcd->state.windowX + 144 + 14)
  {
    if (ppu->lcd->state.ly >= windowY && ppu->lcd->state.ly < windowY + 160)
    {
      uint8_t wTileY = (ppu->state.windowLine / PIXEL_TILE_DIMENSION);

      state.bgwBuffer[0] = ppu->bus->read8(ppu->lcd->getWindowMapArea() + ((state.fetchX + 7 - ppu->lcd->state.windowX) / 8) + (wTileY * BACKGROUND_MAP_DIMENSION));

      if (ppu->lcd->getBgWindowDataArea() == 0x8800)
      {
        state.bgwBuffer[0] += 128;
      }
    }
  }
}

void Pipeline::loadSpriteTile()
{
  for (const auto &entry : ppu->state.currentLineSprites)
  {
    int spriteX = (entry.x - 8) + (ppu->lcd->state.scrollX % 8);
    if ((spriteX >= state.fetchX && spriteX < state.fetchX + 8) ||
        ((spriteX + 8) >= state.fetchX && (spriteX + 8) < state.fetchX + 8))
    {
      state.fetchedEntries[state.entryCount] = entry;
      state.entryCount++;
    }
    if (state.entryCount >= 3)
    {
      break;
    }
  }
}

void Pipeline::loadSpriteData(uint8_t offset)
{
  int curY = ppu->lcd->state.ly;
  uint8_t spriteHeight = ppu->lcd->getObjHeight();

  for (int i = 0; i < state.entryCount; i++)
  {
    uint8_t tileY = ((curY + 16) - state.fetchedEntries[i].y) * 2;

    if (state.fetchedEntries[i].yFlip)
    {
      tileY = ((spriteHeight * 2) - 2) - tileY;
    }

    uint8_t tileIdx = state.fetchedEntries[i].tile;

    if (spriteHeight == 16)
    {
      tileIdx &= ~(1);
    }

    state.objectBuffer[(i * 2) + offset] = ppu->bus->read8(0x8000 + (tileIdx * 16) + tileY + offset);
  }
}

bool Pipeline::fifoAdd()
{
  if (state.fifoSize > 8)
  {
    return false;
  }

  int x = state.fetchX - (8 - (ppu->lcd->state.scrollX % 8));

  for (int i = 0; i < PIXEL_TILE_DIMENSION; i++)
  {
    int bit = 7 - i;
    uint8_t hi = !!(state.bgwBuffer[1] & (1 << bit));
    uint8_t lo = !!(state.bgwBuffer[2] & (1 << bit)) << 1;
    uint32_t color = ppu->lcd->state.bgColors[(hi | lo)];

    if (!(ppu->lcd->isBgWindowEnabled()))
    {
      color = ppu->lcd->state.bgColors[0];
    }

    if (ppu->lcd->isObjEnabled())
    {
      color = fetchSpritePixels(bit, color, hi | lo);
    }

    if (x >= 0)
    {
      fifoPush(color);
      state.fifoX++;
    }
  }
  return true;
}

uint32_t Pipeline::fetchSpritePixels(int bit, uint32_t color, uint8_t bgColor)
{
  for (int i = 0; i < state.entryCount; i++)
  {
    int spriteX = (state.fetchedEntries[i].x - 8) + (ppu->lcd->state.scrollX % 8);

    if ((spriteX + 8) < state.fifoX)
    {
      continue;
    }

    int offset = state.fifoX - spriteX;

    if (offset < 0 || offset >= PIXEL_TILE_DIMENSION)
    {
      continue;
    }

    bit = 7 - offset;

    if (state.fetchedEntries[i].xFlip)
    {
      bit = offset;
    }

    uint8_t hi = !!(state.objectBuffer[i * 2] & (1 << bit));
    uint8_t lo = !!(state.objectBuffer[(i * 2) + 1] & (1 << bit)) << 1;

    bool bgPriority = state.fetchedEntries[i].bgp;

    if (!(hi | lo))
    {
      continue;
    }

    if (!bgPriority || bgColor == 0)
    {
      color = state.fetchedEntries[i].pn ? ppu->lcd->state.ob2Colors[(hi | lo)] : ppu->lcd->state.ob1Colors[(hi | lo)];

      if (hi | lo)
      {
        break;
      }
    }
  }

  return color;
};

void Pipeline::oamReset()
{
  state.fetchState = FETCH_STATE::TILE;
  state.lineX = 0;
  state.fetchX = 0;
  state.pushedCount = 0;
  state.fifoX = 0;
}

uint8_t Pipeline::getPushedCount()
{
  return state.pushedCount;
}

/*
 <--- FIFO-START --->
*/

bool Pipeline::fifoIsEmpty() const
{
  return state.fifoSize == 0;
}

bool Pipeline::fifoIsFull() const
{
  return state.fifoSize == state.pixelFifo.size();
}

void Pipeline::fifoPush(uint32_t pixel)
{
  if (fifoIsFull())
  {
    return;
  }

  state.pixelFifo[state.fifoTail] = pixel;
  state.fifoTail = (state.fifoTail + 1) % state.pixelFifo.size();
  state.fifoSize++;
}

uint32_t Pipeline::fifoPop()
{
  if (fifoIsEmpty())
  {
    return 0;
  }
  uint32_t pixel = state.pixelFifo[state.fifoHead];
  state.fifoHead = (state.fifoHead + 1) % state.pixelFifo.size();
  state.fifoSize--;
  return pixel;
}
/*
 <--- FIFO-END --->
*/