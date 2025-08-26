#include "../../../include/Pipeline.h"
#include "../../../include/Bus.h"
#include "../../../include/Lcd.h"
#include "../../../include/Ppu.h"

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

  // Check Background Window is enabled
  if (ppu->lcd->state.lcdcBits.bgWindowEnablePriority)
  {
    state.bgwBuffer[0] = ppu->bus->read8(bgw0ReadAddress());

    if (ppu->lcd->getBgWindowDataArea() == 0x8800)
    {
      state.bgwBuffer[0] += 128;
    }
    loadWindowTile();
  }

  if (ppu->lcd->state.lcdcBits.objEnable && ppu->state.currentLineSprites.size() > 0)
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

uint8_t Pipeline::bgw0ReadAddress() const
{
  return ppu->lcd->getBgMapArea() + (state.mapX / PIXEL_TILE_DIMENSION) + ((state.mapY / PIXEL_TILE_DIMENSION) * BACKGROUND_MAP_DIMENSION);
}

uint8_t Pipeline::bgw1ReadAddress() const
{
  return ppu->lcd->getBgWindowDataArea() + (state.bgwBuffer[0] * 16) + state.tileY;
}

bool Pipeline::windowVisible() const
{
  return ppu->lcd->state.lcdcBits.windowEnable && (ppu->lcd->state.windowX >= 0) && (ppu->lcd->state.windowX <= 166) && (ppu->lcd->state.windowY >= 0) && (ppu->lcd->state.windowY <= 144);
}

void Pipeline::loadWindowTile()
{
  if (windowVisible())
  {
    return;
  }

  if (state.fetchX + 7 >= ppu->lcd->state.windowX && state.fetchX + 7 < ppu->lcd->state.windowX + 144 + 14)
  {
    if (ppu->lcd->state.ly >= ppu->lcd->state.windowY && ppu->lcd->state.ly < ppu->lcd->state.windowY + 160)
    {
      uint8_t wTileY = (ppu->state.windowLine % PIXEL_TILE_DIMENSION);

      state.bgwBuffer[0] = ppu->bus->read8(ppu->lcd->getWindowMapArea() + ((state.fetchX + 7 - ppu->lcd->state.windowX) / 8) + wTileY * BACKGROUND_MAP_DIMENSION);

      if (ppu->lcd->getBgWindowDataArea() == 0x8800)
      {
        state.bgwBuffer[0] += 128;
      }
    }
  }
}

void Pipeline::loadSpriteTile()
{
  // TODO: Load Sprite Tile
}

void Pipeline::loadSpriteData(uint8_t offset)
{
  // TODO: Load Sprite Data
}

bool Pipeline::fifoAdd()
{
  // TODO: implement fifoAdd
}