#include "../../../include/Pipeline.h"
#include "../../../include/Ppu.h"

Pipeline::Pipeline(PPU *ppu) : ppu(ppu)
{
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