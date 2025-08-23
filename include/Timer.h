#pragma once

#include <cstdint>
#include <memory>

class CPU;
class Timer
{
  struct State
  {
    uint16_t div;
    uint8_t tima;
    uint8_t tma;
    uint8_t tac;
  };

public:
  Timer(std::shared_ptr<CPU> cpu);
  void tick();
  void write(uint16_t address, uint8_t value);
  uint8_t read(uint16_t address);
  void setDiv(uint16_t value);

private:
  State state;
  std::shared_ptr<CPU> cpu;
};