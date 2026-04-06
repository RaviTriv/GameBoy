#pragma once

#include "OamTypes.h"

#include <array>
#include <cstdint>

struct ScanlineContext
{
  uint8_t scrollX = 0;
  uint8_t scrollY = 0;

  uint8_t ly = 0;

  uint8_t windowX = 0;
  uint8_t windowY = 0;
  uint8_t windowLine = 0;

  bool bgWindowEnabled = false;
  bool objEnabled = false;
  bool windowEnabled = false;
  uint8_t objHeight = 0;

  uint16_t bgMapArea = 0;
  uint16_t bgWinDataArea = 0;
  uint16_t winMapArea = 0;

  std::array<uint32_t, 4> bgColors{};
  std::array<uint32_t, 4> ob1Colors{};
  std::array<uint32_t, 4> ob2Colors{};

  const OAM_ENTRY *sprites = nullptr;
  uint8_t spriteCount = 0;

  const uint32_t *lineTicks = nullptr;
};
