#include "Dma.h"

#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>

#include "Common.h"
#include "IMemRead.h"
#include "Ppu.h"
#include "Serializable.h"

void DMA::start(uint8_t start) {
  state.pendingValue = start;
  state.startDelay = START_DELAY;
}

void DMA::tick(IMemRead& memory, PPU& ppu) {
  if (state.isActive) {
    contract_assert(state.byte < OAM_SIZE);
    auto source = static_cast<uint16_t>((state.value * 0x100) + state.byte);
    if (source >= ECHO_RAM_START) {
      source -= 0x2000;
    }
    ppu.oamWrite(state.byte, memory.read8(source));
    state.byte++;
    state.isActive = state.byte < OAM_SIZE;
  }

  if (state.startDelay && --state.startDelay == 0) {
    state.isActive = true;
    state.byte = 0;
    state.value = state.pendingValue;
  }
}

bool DMA::isTransferring() const { return state.isActive; }

bool DMA::blocksOamScan() const {
  return state.isActive || state.startDelay != 0;
}

uint8_t DMA::ppuOamAddress(uint8_t address) const {
  if (!state.isActive || state.byte == 0) {
    return address;
  }
  return static_cast<uint8_t>((state.byte & ~1) | (address & 1));
}
void DMA::save(std::ostream& out) const { serial::saveFields(out, *this); }

void DMA::load(std::istream& in) {
  serial::loadFields(in, *this);
  if (state.isActive &&
      (state.byte >= OAM_SIZE || state.startDelay > START_DELAY)) {
    throw std::runtime_error("Invalid DMA state in save file");
  }
}
