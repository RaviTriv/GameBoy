#pragma once

#include <cstdint>
#include <array>

class PPU;
class Pipeline
{
  enum class FETCH_STATE
  {
    TILE,
    DATA0,
    DATA1,
    IDLE,
    PUSH
  };
  struct State
  {
    FETCH_STATE fetchState;
    std::array<uint32_t, 16> pixelFifo;
    size_t fifoHead = 0;
    size_t fifoTail = 0;
    size_t fifoSize = 0;
  };

public:
  Pipeline(PPU *ppu);
  void process();
  void reset();

private:
  PPU *ppu;
  State state;
  bool fifoIsEmpty() const;
  bool fifoIsFull() const;
  void fifoPush(uint32_t pixel);
  uint32_t fifoPop();
};