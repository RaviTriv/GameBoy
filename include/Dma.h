#pragma once

#include <cstdint>

#include "Serializable.h"

class IMemRead;
class PPU;
class DMA : public Serializable {
  struct State {
    bool isActive = false;
    uint8_t byte = 0;
    uint8_t value = 0;
    uint8_t startDelay = 0;
  };

 public:
  void start(uint8_t start);
  void tick(IMemRead& memory, PPU& ppu);
  [[nodiscard]] bool isTransferring() const;
  void save(std::ostream& out) const override;
  void load(std::istream& in) override;

 private:
  State state;
};