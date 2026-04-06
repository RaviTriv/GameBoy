#pragma once

#include <cstdint>

struct OAM_ENTRY
{
  uint8_t y = 0;
  uint8_t x = 0;
  uint8_t tile = 0;
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
    uint8_t flags = 0;
  };
};