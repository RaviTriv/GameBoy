#pragma once

#include <cstdint>

enum class InterruptType
{
  VBLANK = 1,
  LCD_STAT = 2,
  TIMER = 4,
  SERIAL = 8,
  JOYPAD = 16
};

class InterruptSink
{
public:
  virtual ~InterruptSink() = default;
  virtual void requestInterrupt(InterruptType type) = 0;
};
