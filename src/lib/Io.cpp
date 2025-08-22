#include "../../include/Io.h"

uint8_t IO::read(uint16_t address) const
{
  if (address == 0xFF00)
  {
    // Gamepad
  }

  if (address == 0xFF01)
  {
    return serialData[0];
  }

  if (address == 0xFF02)
  {
    return serialData[1];
  }

  if ((address >= 0xFF04) && (address <= 0xFF07))
  {
    // Timer
  }

  if (address == 0xFF0F)
  {
    // Interrupt flags
  }

  if ((address >= 0xFF10) && (address <= 0xFF26))
  {
    // Apu
  }

  if ((address >= 0xFF30) && (address <= 0xFF3F))
  {
    // Apu
  }

  if ((address >= 0xFF40) && (address <= 0xFF4B))
  {
    // Lcd
  }

  return 0;
}

void IO::write(uint16_t address, uint8_t value)
{
  if (address == 0xFF00)
  {
    // Gamepad
  }

  if (address == 0xFF01)
  {
    serialData[0] = value;
    return;
  }

  if (address == 0xFF02)
  {
    serialData[1] = value;
    return;
  }

  if ((address >= 0xFF04) & (address <= 0xFF07))
  {
    // Timer
  }

  if (address == 0xFF0F)
  {
    // Interrupt flags
  }

  if ((address >= 0xFF10) && (address <= 0xFF26))
  {
    // Apu
  }

  if ((address >= 0xFF30) && (address <= 0xFF3F))
  {
    // Apu
  }

  if ((address >= 0xFF40) && (address <= 0xFF4B))
  {
    // Lcd
  }
}
