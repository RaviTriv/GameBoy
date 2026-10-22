// clang-format Language: Cpp
#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <istream>
#include <ostream>

#include "Interrupts.h"
#include "Serializable.h"

class APU;
class Timer;
class LCD;
class Gamepad;
class IO {
 public:
  IO(Interrupts& interrupts, Timer& timer, LCD& lcd, Gamepad& gamepad,
     APU& apu);
  void write(uint16_t address, uint8_t value);
  [[nodiscard]] uint8_t read(uint16_t address) const;
  void tick();
  void save(std::ostream& out) const;
  void load(std::istream& in);

  void setSerialSink(std::function<void(uint8_t)> sink) {
    serialSink = std::move(sink);
  }

 private:
  [[nodiscard]] bool serialClock() const;

  std::array<char, 2> serialData{};
  uint8_t serialBitsLeft = 0;
  bool serialClockHigh = false;
  [[= serial::skip]] std::function<void(uint8_t)> serialSink;
  [[= serial::skip]] Interrupts& interrupts_;
  [[= serial::skip]] APU& apu_;
  [[= serial::skip]] Timer& timer_;
  [[= serial::skip]] LCD& lcd_;
  [[= serial::skip]] Gamepad& gamepad_;

  static constexpr uint8_t SERIAL_BITS = 8;
  static constexpr uint8_t SERIAL_INTERNAL_CLOCK = 0x01;
  static constexpr uint8_t SERIAL_CLOCK_DIV_BIT = 0x01;
};