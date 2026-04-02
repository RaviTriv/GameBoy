#pragma once

#include <cstdint>

enum class InterruptType
{
  VBLANK = 1,
  LCD_STAT = 2,
  TIMER = 4,
  SERIAL = 8,
  JOYPAD = 16
};

class CpuContext;
class Interrupts
{
public:
  Interrupts(CpuContext *ctx);

  void requestInterrupt(InterruptType type);
  void handleInterrupts();
  void setInterruptEnable(uint8_t value);
  [[nodiscard]] uint8_t getInterruptEnable() const;

private:
  CpuContext *ctx;
  void interruptHandle(uint16_t address);
  bool checkInterrupt(uint16_t address, InterruptType type);
};
