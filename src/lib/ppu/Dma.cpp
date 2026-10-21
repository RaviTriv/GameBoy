#include "Dma.h"

#include <cstdint>
#include <istream>
#include <ostream>

#include "IMemRead.h"
#include "Ppu.h"
#include "Serializable.h"

void DMA::start(uint8_t start) {
  state.isActive = true;
  state.byte = 0;
  state.value = start;
  state.startDelay = 2;
}

void DMA::tick(IMemRead& memory, PPU& ppu) {
  if (!state.isActive) {
    return;
  }

  if (state.startDelay) {
    state.startDelay--;
    return;
  }

  ppu.oamWrite(
      state.byte,
      memory.read8(static_cast<uint16_t>((state.value * 0x100) + state.byte)));
  state.byte++;
  state.isActive = state.byte < 0xA0;
}

bool DMA::isTransferring() const { return state.isActive; }
void DMA::save(std::ostream& out) const { serial::saveFields(out, *this); }

void DMA::load(std::istream& in) { serial::loadFields(in, *this); }
