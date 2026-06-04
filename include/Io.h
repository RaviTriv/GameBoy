#pragma once

#include <array>
#include <cstdint>
#include <functional>

#include "InterruptRegs.h"

class APU;
class Timer;
class LCD;
class Gamepad;
class IO {
 public:
  IO(InterruptRegs interruptRegs, Timer &timer, LCD &lcd, Gamepad &gamepad,
     APU &apu);
  void write(uint16_t address, uint8_t value);
  [[nodiscard]] uint8_t read(uint16_t address) const;

  void setSerialSink(std::function<void(uint8_t)> sink) {
    serialSink = std::move(sink);
  }

 private:
  std::array<char, 2> serialData{};
  std::function<void(uint8_t)> serialSink;
  InterruptRegs interruptRegs;
  APU &apu;
  Timer &timer;
  LCD &lcd;
  Gamepad &gamepad;
};