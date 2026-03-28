#include "../../../include/Dma.h"
#include "../../../include/IMemRead.h"

DMA::DMA(std::function<void(uint16_t, uint8_t)> oamWrite) : oamWrite(std::move(oamWrite))
{
}

void DMA::start(uint8_t start)
{
  state.isActive = true;
  state.byte = 0;
  state.value = start;
  state.startDelay = 2;
}

void DMA::tick()
{
  if (!state.isActive)
  {
    return;
  }

  if (state.startDelay)
  {
    state.startDelay--;
    return;
  }

  oamWrite(state.byte, memRead->read8((state.value * 0x100) + state.byte));
  state.byte++;
  state.isActive = state.byte < 0xA0;
}

bool DMA::isTransferring() const
{
  return state.isActive;
}