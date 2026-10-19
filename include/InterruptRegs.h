#pragma once

#include <cstdint>

enum class InterruptType : uint8_t {
  VBLANK = 1,
  LCD_STAT = 2,
  TIMER = 4,
  SERIAL = 8,
  JOYPAD = 16
};

struct InterruptRegs {
  uint8_t &ie;
  uint8_t &intf;

  void request(InterruptType type) const {
    intf |= static_cast<uint8_t>(type);
  }
};
