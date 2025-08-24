#include "../../../include/Dma.h"
#include "../../../include/Bus.h"
#include "../../../include/PPU.h"

DMA::DMA(std::shared_ptr<Bus> bus, std::shared_ptr<PPU> ppu) : bus(bus), ppu(ppu)
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

  ppu->oamWrite(state.byte, bus->read8((state.value * 0x100) + state.byte));
  state.byte++;
  state.isActive = state.byte < 0xA0;
}

bool DMA::isTransferring() const
{
  return state.isActive;
}