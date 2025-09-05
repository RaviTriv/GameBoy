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

private:
  State state;
  std::shared_ptr<CPU> cpu;
  static constexpr uint16_t INITIAL_DIV_VALUE = 0xAC00;
  static constexpr uint16_t DIV_REGISTER = 0xFF04;
  static constexpr uint16_t TIMA_REGISTER = 0xFF05;
  static constexpr uint16_t TMA_REGISTER = 0xFF06;
  static constexpr uint16_t TAC_REGISTER = 0xFF07;
  static constexpr uint8_t TIMER_ENABLE_BIT = (1 << 2);
  static constexpr uint8_t FREQUENCY_BITS_MASK = 0b11;
  static constexpr uint8_t TIMA_OVERFLOW = 0xFF;
};