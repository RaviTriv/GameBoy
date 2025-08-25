#pragma once

#include "./Pipeline.h"

#include <array>
#include <cstdint>
#include <memory>
#include <list>

class CPU;
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
    uint8_t windowLine;
    std::list<OAM_ENTRY> currentLineSprites;
  };

public:
  void tick();
  PPU(std::shared_ptr<CPU> cpu, std::shared_ptr<LCD> lcd);

  void oamWrite(uint16_t addr, uint8_t value);
  uint8_t oamRead(uint16_t addr);
  uint8_t vramRead(uint16_t address) const;
  void vramWrite(uint16_t address, uint8_t value);

  void setCpu(std::shared_ptr<CPU> cpu);

private:
  State state;
  std::shared_ptr<CPU> cpu;
  std::shared_ptr<LCD> lcd;
  friend class Pipeline;
  Pipeline pipeline;

  static constexpr uint16_t VRAM_START_ADDR = 0x8000;
  static constexpr uint16_t OAM_START_ADDR = 0xFE00;
  static constexpr int LINES_PER_FRAME = 154;
  static constexpr int TICKS_PER_LINE = 456;
  static constexpr int XRES = 160;
  static constexpr int YRES = 144;

  void incrementLY();
  void loadLineSprites();

  void oamMode();
  void drawingMode();
  void hBlankMode();
  void vBlankMode();
};