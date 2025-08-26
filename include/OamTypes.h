#pragma once

#include <cstdint>

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