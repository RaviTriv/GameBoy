#pragma once

#include <cstdint>

#include "Common.h"

struct Registers {
  uint8_t a = 0;
  uint8_t f = 0;
  uint8_t b = 0;
  uint8_t c = 0;
  uint8_t d = 0;
  uint8_t e = 0;
  uint8_t h = 0;
  uint8_t l = 0;
  uint16_t pc = 0;
  uint16_t sp = 0;

  [[nodiscard]] uint16_t hl() const {
    return static_cast<uint16_t>((h << 8) | l);
  }
  [[nodiscard]] uint16_t bc() const {
    return static_cast<uint16_t>((b << 8) | c);
  }
  [[nodiscard]] uint16_t de() const {
    return static_cast<uint16_t>((d << 8) | e);
  }
  [[nodiscard]] uint16_t af() const {
    return static_cast<uint16_t>((a << 8) | (f & HIGH_NIBBLE_MASK));
  }

  void setHl(uint16_t v) {
    h = highByte(v);
    l = lowByte(v);
  }
  void setBc(uint16_t v) {
    b = highByte(v);
    c = lowByte(v);
  }
  void setDe(uint16_t v) {
    d = highByte(v);
    e = lowByte(v);
  }
  void setAf(uint16_t v) {
    a = highByte(v);
    f = static_cast<uint8_t>(v & HIGH_NIBBLE_MASK);
  }
};
