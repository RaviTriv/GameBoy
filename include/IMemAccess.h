#pragma once

#include "IMemRead.h"

class IMemAccess : public IMemRead {
 public:
  virtual void write8(uint16_t address, uint8_t value) = 0;
  virtual void write16(uint16_t address, uint16_t value) = 0;
};
