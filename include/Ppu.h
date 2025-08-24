#pragma once

#include "./Lcd.h"

#include <array>
#include <cstdint>
#include <memory>

class LCD;
class PPU
{
  struct OAM_ENTRY
  {
    uint8_t y;
    uint8_t x;
    uint8_t tile;
    union
    {
      struct
      {
        uint8_t cgbPn : 3;
        uint8_t cgbVramBank : 1;
        uint8_t pn : 1;
        uint8_t xFlip : 1;
        uint8_t yFlip : 1;
        uint8_t bgp : 1;
      };
      uint8_t flags;
    };
  };

  struct State
  {
    std::array<OAM_ENTRY, 40> oamRam;
    std::array<uint8_t, 0x2000> vram;

    uint32_t lineTicks;
  };

public:
  void tick();

private:
  State state;
  friend class LCD;
  LCD lcd;

  static constexpr uint16_t VRAM_START_ADDR = 0x8000;
  static constexpr uint16_t OAM_START_ADDR = 0xFE00;

  void oamWrite(uint16_t addr, uint8_t value);
  uint8_t oamRead(uint16_t addr);
  uint8_t vramRead(uint16_t address) const;
  void vramWrite(uint16_t address, uint8_t value);
};