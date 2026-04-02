#pragma once

#include <cstdint>
#include <functional>

class IMemRead;
class DMA
{
  struct State
  {
    bool isActive;
    uint8_t byte;
    uint8_t value;
    uint8_t startDelay;
  };

public:
  DMA(std::function<void(uint16_t, uint8_t)> oamWrite);
  void start(uint8_t start);
  void tick();
  [[nodiscard]] bool isTransferring() const;
  void setMemRead(IMemRead &memRead) { this->memRead = &memRead; }
private:
  State state;
  IMemRead *memRead = nullptr;
  std::function<void(uint16_t, uint8_t)> oamWrite;
};