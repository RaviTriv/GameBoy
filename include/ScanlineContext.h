// clang-format Language: Cpp
#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "OamTypes.h"
#include "Serializable.h"

class DMA;

struct ScanlineContext {
  uint8_t scrollX = 0;
  uint8_t scrollY = 0;

  uint8_t ly = 0;

  uint8_t windowX = 0;
  uint8_t windowY = 0;

  bool bgWindowEnabled = false;
  bool objEnabled = false;
  bool windowEnabled = false;
  uint8_t objHeight = 0;

  uint16_t bgMapArea = 0;
  uint16_t bgWinDataArea = 0;
  uint16_t winMapArea = 0;

  uint8_t bgPalette = 0;
  uint8_t obj0Palette = 0;
  uint8_t obj1Palette = 0;

  [[= serial::skip]] std::span<const LineSprite> sprites;
  [[= serial::skip]] std::span<const uint8_t> oam;
  [[= serial::skip]] const DMA* dma = nullptr;

  [[= serial::skip]] const uint32_t* lineTicks = nullptr;
};
