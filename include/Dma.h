#pragma once

#include <cstdint>

#include "Serializable.h"

class IMemRead;
class PPU;
class DMA {
  static constexpr uint8_t OAM_SIZE = 0xA0;
  static constexpr uint8_t START_DELAY = 2;

  struct State {
    bool isActive = false;
    uint8_t byte = 0;
    uint8_t value = 0;
    uint8_t startDelay = 0;
    uint8_t pendingValue = 0;
  };

 public:
  void start(uint8_t start);
  void tick(IMemRead& memory, PPU& ppu);
  [[nodiscard]] bool isTransferring() const;
  [[nodiscard]] bool blocksOamScan() const;
  [[nodiscard]] uint8_t ppuOamAddress(uint8_t address) const;
  void save(std::ostream& out) const;
  void load(std::istream& in);

 private:
  State state;
};