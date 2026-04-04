#pragma once

#include <cstdint>
#include <functional>

class IMemRead;
class DMA
{
  struct State
  {
    bool isActive = false;
    uint8_t byte = 0;
    uint8_t value = 0;
    uint8_t startDelay = 0;
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