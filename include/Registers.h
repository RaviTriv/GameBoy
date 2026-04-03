#pragma once

#include <cstdint>
#include "Common.h"

struct Registers {
  uint8_t a;
  uint8_t f;
  uint8_t b;
  uint8_t c;
  uint8_t d;
  uint8_t e;
  uint8_t h;
  uint8_t l;
  uint16_t pc;
  uint16_t sp;

  [[nodiscard]] uint16_t hl() const { return (static_cast<uint16_t>(h) << 8) | l; }
  [[nodiscard]] uint16_t bc() const { return (static_cast<uint16_t>(b) << 8) | c; }
  [[nodiscard]] uint16_t de() const { return (static_cast<uint16_t>(d) << 8) | e; }
  [[nodiscard]] uint16_t af() const { return (static_cast<uint16_t>(a) << 8) | (f & HIGH_NIBBLE_MASK); }

  void setHl(uint16_t v) { h = (v >> 8) & BYTE_MASK; l = v & BYTE_MASK; }
  void setBc(uint16_t v) { b = (v >> 8) & BYTE_MASK; c = v & BYTE_MASK; }
  void setDe(uint16_t v) { d = (v >> 8) & BYTE_MASK; e = v & BYTE_MASK; }
  void setAf(uint16_t v) { a = (v >> 8) & BYTE_MASK; f = v & HIGH_NIBBLE_MASK; }
};
