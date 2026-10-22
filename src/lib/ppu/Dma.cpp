#include "Dma.h"

#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>

#include "IMemRead.h"
#include "Ppu.h"
#include "Serializable.h"

void DMA::start(uint8_t start) {
  state.isActive = true;
  state.byte = 0;
  state.value = start;
  state.startDelay = START_DELAY;
}

void DMA::tick(IMemRead& memory, PPU& ppu) {
  if (!state.isActive) {
    return;
  }

  if (state.startDelay) {
    state.startDelay--;
    return;
  }

  contract_assert(state.byte < OAM_SIZE);
  ppu.oamWrite(
      state.byte,
      memory.read8(static_cast<uint16_t>((state.value * 0x100) + state.byte)));
  state.byte++;
  state.isActive = state.byte < OAM_SIZE;
}

bool DMA::isTransferring() const { return state.isActive; }
void DMA::save(std::ostream& out) const { serial::saveFields(out, *this); }

void DMA::load(std::istream& in) {
  serial::loadFields(in, *this);
  if (state.isActive &&
      (state.byte >= OAM_SIZE || state.startDelay > START_DELAY)) {
    throw std::runtime_error("Invalid DMA state in save file");
  }
}
