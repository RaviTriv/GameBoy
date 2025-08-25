#include "../../../include/Pipeline.h"
#include "../../../include/Ppu.h"

Pipeline::Pipeline(PPU *ppu) : ppu(ppu)
{
}

void Pipeline::process()
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