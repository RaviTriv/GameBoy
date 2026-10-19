#pragma once

#include <cstdint>
#include <functional>

#include "Serializable.h"

class IMemRead;
class DMA : public Serializable {
  struct State {
    bool isActive = false;
    uint8_t byte = 0;
    uint8_t value = 0;
    uint8_t startDelay = 0;
  };

 public:
  explicit DMA(std::function<void(uint16_t, uint8_t)> oamWrite);
  void start(uint8_t start);
  void tick();
  [[nodiscard]] bool isTransferring() const;
  void save(std::ostream& out) const override;
  void load(std::istream& in) override;
  void setMemRead(IMemRead& reader) { memRead = &reader; }

 private:
  State state;
  IMemRead* memRead = nullptr;
  std::function<void(uint16_t, uint8_t)> oamWrite_;
};