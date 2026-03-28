#pragma once

#include <cstdint>

struct InterruptRegs
{
  uint8_t &ie;
  uint8_t &intf;
};
