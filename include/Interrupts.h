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

class CPU;
class Interrupts
{
public:
  Interrupts(CPU *cpu);

  void requestInterrupt(InterruptType type);
  void handleInterrupts();
  void setInterruptEnable(uint8_t value);
  uint8_t getInterruptEnable() const;

private:
  CPU *cpu;
  void interruptHandle(uint16_t address);
  bool checkInterrupt(uint16_t address, InterruptType type);
};