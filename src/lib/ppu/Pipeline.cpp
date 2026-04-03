#include "Pipeline.h"
#include "Common.h"
#include "Logger.h"

Pipeline::Pipeline(ReadFn readFn, WritePixelFn writePixelFn)
    : readFn(std::move(readFn)), writePixelFn(std::move(writePixelFn))
{
}

void Pipeline::beginScanline(const ScanlineContext &scanlineCtx)
{
  ctx = &scanlineCtx;
}

void Pipeline::process()
{
  if ((*ctx->lineTicks & 1) == 0)
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
    fetchDataLow();
    break;
  case FETCH_STATE::DATA1:
    fetchDataHigh();
    break;
  case FETCH_STATE::IDLE:
    state.fetchState = FETCH_STATE::PUSH;
    break;
  case FETCH_STATE::PUSH:
    if (processTile())
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
  state.mapY = calculateMapY();
  state.mapX = calculateMapX();
  state.tileY = calculateTileY();

  state.entryCount = 0;

  if (ctx->bgWindowEnabled)
  {
    state.bgwBuffer[0] = readFn(bgw0ReadAddress());

    loadWindowTile();

    if (ctx->bgWinDataArea == 0x8800)
    {
      state.bgwBuffer[0] += 128;
    }
  }

  if (ctx->objEnabled && ctx->spriteCount > 0)
  {
    loadSpriteTile();
  }

  state.fetchState = FETCH_STATE::DATA0;
  state.fetchX += 8;
}

void Pipeline::fetchDataLow()
{
  state.bgwBuffer[1] = readFn(bgw1ReadAddress());
  loadSpriteData(0);
  state.fetchState = FETCH_STATE::DATA1;
}

void Pipeline::fetchDataHigh()
{
  state.bgwBuffer[2] = readFn(bgw1ReadAddress() + 1);
  loadSpriteData(1);
  state.fetchState = FETCH_STATE::IDLE;
}

bool Pipeline::processTile()
{
  if (pixelFifo.size() > 8)
  {
    return false;
  }

  int x = state.fetchX - (8 - (ctx->scrollX % 8));

  for (int i = 0; i < PIXEL_TILE_DIMENSION; i++)
  {
    int bit = 7 - i;
    uint8_t hi = !!(state.bgwBuffer[1] & (1 << bit));
    uint8_t lo = !!(state.bgwBuffer[2] & (1 << bit)) << 1;
    uint32_t color = ctx->bgColors[(hi | lo)];

    if (!ctx->bgWindowEnabled)
    {
      color = ctx->bgColors[0];
    }

    if (ctx->objEnabled)
    {
      color = fetchSpritePixels(bit, color, hi | lo);
    }

    if (x >= 0)
    {
      pixelFifo.push(color);
      state.fifoX++;
    }
  }
  return true;
}

void Pipeline::pushPixel()
{
  if (pixelFifo.size() > 8)
  {
    uint32_t pixel = pixelFifo.pop();

    if (state.lineX >= (ctx->scrollX) % 8)
    {
      writePixelFn(bufferIndex(), pixel);
      state.pushedCount++;
    }

    state.lineX++;
  }
}

uint32_t Pipeline::fetchSpritePixels(int bit, uint32_t color, uint8_t bgColor)
{
  for (int i = 0; i < state.entryCount; i++)
  {
    int spriteX = (state.fetchedEntries[i].x - 8) + (ctx->scrollX % 8);

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
      color = state.fetchedEntries[i].pn ? ctx->ob2Colors[(hi | lo)] : ctx->ob1Colors[(hi | lo)];

      if (hi | lo)
      {
        break;
      }
    }
  }

  return color;
};

/*
 <--- LOAD-START --->
*/

void Pipeline::loadWindowTile()
{
  if (!isWindowVisible())
  {
    return;
  }

  uint8_t windowY = ctx->windowY;

  if (state.fetchX + 7 >= ctx->windowX && state.fetchX + 7 < ctx->windowX + YRES + 14)
  {
    if (ctx->ly >= windowY && ctx->ly < windowY + XRES)
    {
      uint8_t wTileY = (ctx->windowLine / PIXEL_TILE_DIMENSION);

      state.bgwBuffer[0] = readFn(windowTileReadAddress(wTileY));
    }
  }
}

void Pipeline::loadSpriteTile()
{
  for (uint8_t s = 0; s < ctx->spriteCount; s++)
  {
    const auto &entry = ctx->sprites[s];
    int spriteX = (entry.x - 8) + (ctx->scrollX % 8);
    if ((spriteX >= state.fetchX && spriteX < state.fetchX + 8) ||
        ((spriteX + 8) >= state.fetchX && (spriteX + 8) < state.fetchX + 8))
    {
      state.fetchedEntries[state.entryCount] = entry;
      state.entryCount++;
    }
    if (state.entryCount >= 10)
    {
      break;
    }
  }
}

void Pipeline::loadSpriteData(uint8_t offset)
{
  int curY = ctx->ly;
  uint8_t spriteHeight = ctx->objHeight;

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

    state.objectBuffer[(i * 2) + offset] = readFn(0x8000 + (tileIdx * 16) + tileY + offset);
  }
}

/*
 <--- LOAD-END --->
*/

/*
<--- CALCULATE-POSITION-START --->
*/

uint8_t Pipeline::calculateMapY() const
{
  return ctx->ly + ctx->scrollY;
}

uint8_t Pipeline::calculateMapX() const
{
  return state.fetchX + ctx->scrollX;
}

uint8_t Pipeline::calculateTileY() const
{
  return ((ctx->ly + ctx->scrollY) % 8) * 2;
}

/*
<--- CALCULATE-POSITION-END --->
*/

/*
<--- CALCULATE-ADDRESS-START --->
*/

uint32_t Pipeline::bufferIndex() const
{
  return state.pushedCount + ctx->ly * XRES;
}

uint16_t Pipeline::bgw0ReadAddress() const
{
  return ctx->bgMapArea + (state.mapX / PIXEL_TILE_DIMENSION) + (((state.mapY / PIXEL_TILE_DIMENSION)) * BACKGROUND_MAP_DIMENSION);
}

uint16_t Pipeline::bgw1ReadAddress() const
{
  return ctx->bgWinDataArea + (state.bgwBuffer[0] * 16) + state.tileY;
}

uint16_t Pipeline::windowTileReadAddress(uint8_t wTileY) const
{
  return ctx->winMapArea +
         ((state.fetchX + 7 - ctx->windowX) / 8) +
         (wTileY * BACKGROUND_MAP_DIMENSION);
}
/*
<--- CALCULATE-ADDRESS-END --->
*/

bool Pipeline::isWindowVisible() const
{
  if (!ctx)
  {
    return false;
  }
  return ctx->windowEnabled && ctx->windowX <= 166 && ctx->windowY <= YRES;
}

/*
<--- RESET-START --->
*/

void Pipeline::reset()
{
  while (!pixelFifo.isEmpty())
  {
    pixelFifo.pop();
  }
}

void Pipeline::oamReset()
{
  state.fetchState = FETCH_STATE::TILE;
  state.lineX = 0;
  state.fetchX = 0;
  state.pushedCount = 0;
  state.fifoX = 0;
}

/*
<--- RESET-END --->
*/
