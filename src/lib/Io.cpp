#include "../../include/Io.h"
#include "../../include/Apu.h"
#include "../../include/Common.h"
#include "../../include/Cpu.h"
#include "../../include/Lcd.h"
#include "../../include/Timer.h"
#include "../../include/Gamepad.h"
#include "../../include/Logger.h"

IO::IO(std::shared_ptr<CPU> cpu, std::shared_ptr<Timer> timer, std::shared_ptr<LCD> lcd, std::shared_ptr<Gamepad> gamepad, std::shared_ptr<APU> apu) : cpu(cpu), timer(timer), lcd(lcd), apu(apu)
{
}

void IO::setApu(std::shared_ptr<APU> apu) { this->apu = apu; }
void IO::setTimer(std::shared_ptr<Timer> timer) { this->timer = timer; }
void IO::setCPU(std::shared_ptr<CPU> cpu) { this->cpu = cpu; }
void IO::setLCD(std::shared_ptr<LCD> lcd) { this->lcd = lcd; }
void IO::setGamepad(std::shared_ptr<Gamepad> gamepad) { this->gamepad = gamepad; }

uint8_t IO::read(uint16_t address) const
{
  if (address == JOYPAD_REGISTER)
  {
    // Gamepad
    return gamepad->getOutput();
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
    return timer->read(address);
  }

  if (address == IF_REGISTER)
  {
    // Interrupt flags
    return cpu->getInterruptFlags();
  }

  if ((address >= APU_START) && (address <= APU_END))
  {
    // Apu
    return apu->read(address);
  }

  if ((address >= WAVE_RAM_START) && (address <= WAVE_RAM_END))
  {
    // Apu
    return apu->read(address);
  }

  if ((address >= LCD_START) && (address <= LCD_END))
  {
    // Lcd
    return lcd->read(address);
  }

  return 0;
}

void IO::write(uint16_t address, uint8_t value)
{
  if (address == JOYPAD_REGISTER)
  {
    // Gamepad
    gamepad->setSel(value);
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
    timer->write(address, value);
    return;
  }

  if (address == IF_REGISTER)
  {
    // Interrupt flags
    cpu->setInterruptFlags(value);
    return;
  }

  if ((address >= APU_START) && (address <= APU_END))
  {
    // Apu
    apu->write(address, value);
    return;
  }

  if ((address >= WAVE_RAM_START) && (address <= WAVE_RAM_END))
  {
    // Apu
    apu->write(address, value);
    return;
  }

  if ((address >= LCD_START) && (address <= LCD_END))
  {
    // Lcd
    return lcd->write(address, value);
  }
}