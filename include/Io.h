#pragma once

#include "InterruptRegs.h"
#include <array>
#include <cstdint>

class APU;
class Timer;
class LCD;
class Gamepad;
class IO
{
public:
  IO(InterruptRegs interruptRegs, Timer &timer, LCD &lcd, Gamepad &gamepad, APU &apu);
  void write(uint16_t address, uint8_t value);
  [[nodiscard]] uint8_t read(uint16_t address) const;

private:
  std::array<char, 2> serialData{};
  InterruptRegs interruptRegs;
  APU &apu;
  Timer &timer;
  LCD &lcd;
  Gamepad &gamepad;
};