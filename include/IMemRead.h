#pragma once

#include <cstdint>

class IMemRead
{
public:
  virtual ~IMemRead() = default;
  [[nodiscard]] virtual uint8_t read8(uint16_t address) = 0;
};
