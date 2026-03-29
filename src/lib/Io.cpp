#include "../../include/Io.h"
#include "../../include/Apu.h"
#include "../../include/Common.h"
#include "../../include/Lcd.h"
#include "../../include/Timer.h"
#include "../../include/Gamepad.h"
#include "../../include/Logger.h"

IO::IO(InterruptRegs interruptRegs, Timer &timer, LCD &lcd, Gamepad &gamepad, APU &apu) : interruptRegs(interruptRegs), apu(apu), timer(timer), lcd(lcd), gamepad(gamepad)
{
}

uint8_t IO::read(uint16_t address)
{
  if (address == JOYPAD_REGISTER)
  {
    // Gamepad
    return gamepad.getOutput();
  }

  if (address == SERIAL_DATA_REGISTER)
  {
    return serialData[SERIAL_DATA_INDEX];
  }

  if (address == SERIAL_CONTROL_REGISTER)
  {
    return serialData[SERIAL_CONTROL_INDEX];
  }

  if ((address >= TIMER_START) && (address <= TIMER_END))
  {
    // Timer
    return timer.read(address);
  }

  if (address == IF_REGISTER)
  {
    // Interrupt flags
    return interruptRegs.intf;
  }

  if ((address >= APU_START) && (address <= APU_END))
  {
    // Apu
    return apu.read(address);
  }

  if ((address >= WAVE_RAM_START) && (address <= WAVE_RAM_END))
  {
    // Apu
    return apu.read(address);
  }

  if ((address >= LCD_START) && (address <= LCD_END))
  {
    // Lcd
    return lcd.read(address);
  }

  return 0;
}

void IO::write(uint16_t address, uint8_t value)
{
  if (address == JOYPAD_REGISTER)
  {
    // Gamepad
    gamepad.setSel(value);
    return;
  }

  if (address == SERIAL_DATA_REGISTER)
  {
    serialData[SERIAL_DATA_INDEX] = value;
    return;
  }

  if (address == SERIAL_CONTROL_REGISTER)
  {
    serialData[SERIAL_CONTROL_INDEX] = value;
    return;
  }

  if ((address >= TIMER_START) && (address <= TIMER_END))
  {
    // Timer
    timer.write(address, value);
    return;
  }

  if (address == IF_REGISTER)
  {
    // Interrupt flags
    interruptRegs.intf = value;
    return;
  }

  if ((address >= APU_START) && (address <= APU_END))
  {
    // Apu
    apu.write(address, value);
    return;
  }

  if ((address >= WAVE_RAM_START) && (address <= WAVE_RAM_END))
  {
    // Apu
    apu.write(address, value);
    return;
  }

  if ((address >= LCD_START) && (address <= LCD_END))
  {
    // Lcd
    return lcd.write(address, value);
  }
}