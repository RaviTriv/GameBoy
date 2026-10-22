#pragma once

#include "IMemRead.h"

class IMemAccess : public IMemRead {
 public:
  virtual void write8(uint16_t address, uint8_t value) = 0;
  [[nodiscard]] virtual uint8_t read8WithIdu(uint16_t address) = 0;
  virtual void idu(uint16_t address) = 0;
};
