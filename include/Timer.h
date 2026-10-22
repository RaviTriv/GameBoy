// clang-format Language: Cpp
#pragma once

#include <array>
#include <cstdint>

#include "Interrupts.h"
#include "Serializable.h"

class Timer {
  enum class Reload : uint8_t { NONE, PENDING, DONE };

  struct State {
    uint16_t div = 0;
    uint8_t tima = 0;
    uint8_t tma = 0;
    uint8_t tac = 0;
    Reload reload = Reload::NONE;
  };

 public:
  explicit Timer(Interrupts& interrupts);
  void tick();
  void write(uint16_t address, uint8_t value);
  [[nodiscard]] uint8_t read(uint16_t address) const;
  void save(std::ostream& out) const;
  void load(std::istream& in);

 private:
  [[nodiscard]] bool timerSignal() const;
  void incrementTima();
  void advanceReload();

  State state;
  [[= serial::skip]] Interrupts& interrupts_;
  static constexpr uint16_t INITIAL_DIV_VALUE = 0xABCC;
  static constexpr uint16_t DIV_REGISTER = 0xFF04;
  static constexpr uint16_t TIMA_REGISTER = 0xFF05;
  static constexpr uint16_t TMA_REGISTER = 0xFF06;
  static constexpr uint16_t TAC_REGISTER = 0xFF07;
  static constexpr uint8_t TIMER_ENABLE_BIT = (1 << 2);
  static constexpr uint8_t FREQUENCY_BITS_MASK = 0b11;
  static constexpr uint8_t TAC_UNUSED_BITS = 0xF8;
  static constexpr uint16_t M_CYCLE_MASK = 0b11;
  static constexpr std::array<uint16_t, 4> FREQUENCY_BITS = {1 << 9, 1 << 3,
                                                             1 << 5, 1 << 7};
};