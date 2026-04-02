#pragma once

#include "IMemRead.h"

class IMemAccess : public IMemRead
{
public:
  [[nodiscard]] virtual uint16_t read16(uint16_t address) = 0;
  virtual void write8(uint16_t address, uint8_t value) = 0;
  virtual void write16(uint16_t address, uint16_t value) = 0;
};
