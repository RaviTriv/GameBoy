#pragma once

#include "OamTypes.h"

#include <array>
#include <cstdint>

struct ScanlineContext
{
  uint8_t scrollX;
  uint8_t scrollY;

  uint8_t ly;

  uint8_t windowX;
  uint8_t windowY;
  uint8_t windowLine;

  bool bgWindowEnabled;
  bool objEnabled;
  bool windowEnabled;
  uint8_t objHeight;

  uint16_t bgMapArea;
  uint16_t bgWinDataArea;
  uint16_t winMapArea;

  std::array<uint32_t, 4> bgColors;
  std::array<uint32_t, 4> ob1Colors;
  std::array<uint32_t, 4> ob2Colors;

  const OAM_ENTRY *sprites;
  uint8_t spriteCount;

  const uint32_t *lineTicks;
};
