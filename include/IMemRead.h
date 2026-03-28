#pragma once

#include <cstdint>

class IMemRead
{
public:
  virtual ~IMemRead() = default;
  virtual uint8_t read8(uint16_t address) = 0;
};
