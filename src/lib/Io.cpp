#include "../../include/Io.h"
#include "../../include/Cpu.h"
#include "../../include/Lcd.h"
#include "../../include/Timer.h"

IO::IO(std::shared_ptr<CPU> cpu, std::shared_ptr<Timer> timer, std::shared_ptr<LCD> lcd) : cpu(cpu), timer(timer), lcd(lcd)
{
}

void IO::setTimer(std::shared_ptr<Timer> timer) { this->timer = timer; }
void IO::setCPU(std::shared_ptr<CPU> cpu) { this->cpu = cpu; }
void IO::setLCD(std::shared_ptr<LCD> lcd) { this->lcd = lcd; }

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
    return timer->read(address);
  }

  if (address == 0xFF0F)
  {
    // Interrupt flags
    return cpu->getInterruptFlags();
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
    return lcd->read(address);
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
    timer->write(address, value);
    return;
  }

  if (address == 0xFF0F)
  {
    // Interrupt flags
    cpu->setInterruptFlags(value);
    return;
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
    return lcd->write(address, value);
  }
}
