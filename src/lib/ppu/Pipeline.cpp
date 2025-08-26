#include "../../../include/Pipeline.h"
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
    break;
  case FETCH_STATE::DATA0:
    break;
  case FETCH_STATE::DATA1:
    break;
  case FETCH_STATE::IDLE:
    break;
  case FETCH_STATE::PUSH:
    break;
  default:
    throw std::runtime_error("Trying to process unknown fetch state");
    break;
  }
}

void Pipeline::pushPixel()
{
  if (state.fifoSize > 8)
  {
    uint32_t pixel = fifoPop();

    if (state.lineX >= (ppu->lcd->state.scrollX) % 8)
    {
      // TODO: Push to video buffer
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
