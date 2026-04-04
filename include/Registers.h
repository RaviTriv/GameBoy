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

  [[nodiscard]] uint16_t hl() const { return (static_cast<uint16_t>(h) << 8) | l; }
  [[nodiscard]] uint16_t bc() const { return (static_cast<uint16_t>(b) << 8) | c; }
  [[nodiscard]] uint16_t de() const { return (static_cast<uint16_t>(d) << 8) | e; }
  [[nodiscard]] uint16_t af() const { return (static_cast<uint16_t>(a) << 8) | (f & HIGH_NIBBLE_MASK); }

  void setHl(uint16_t v) { h = (v >> 8) & BYTE_MASK; l = v & BYTE_MASK; }
  void setBc(uint16_t v) { b = (v >> 8) & BYTE_MASK; c = v & BYTE_MASK; }
  void setDe(uint16_t v) { d = (v >> 8) & BYTE_MASK; e = v & BYTE_MASK; }
  void setAf(uint16_t v) { a = (v >> 8) & BYTE_MASK; f = v & HIGH_NIBBLE_MASK; }
};
