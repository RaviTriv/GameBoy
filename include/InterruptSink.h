#pragma once

#include "Interrupts.h"

class InterruptSink
{
public:
  virtual ~InterruptSink() = default;
  virtual void requestInterrupt(InterruptType type) = 0;
};
