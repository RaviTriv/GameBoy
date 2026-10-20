#include "Io.h"

#include <cstdint>

#include "Apu.h"
#include "Common.h"
#include "Gamepad.h"
#include "Interrupts.h"
#include "Lcd.h"
#include "Timer.h"

IO::IO(Interrupts& interrupts, Timer& timer, LCD& lcd, Gamepad& gamepad,
       APU& apu)
    : interrupts_(interrupts),
      apu_(apu),
      timer_(timer),
      lcd_(lcd),
      gamepad_(gamepad) {}

uint8_t IO::read(uint16_t address) const {
  if (address == JOYPAD_REGISTER) {
    // Gamepad
    return gamepad_.getOutput();
  }

  if (address == SERIAL_DATA_REGISTER) {
    return static_cast<uint8_t>(serialData[SERIAL_DATA_INDEX]);
  }

  if (address == SERIAL_CONTROL_REGISTER) {
    return static_cast<uint8_t>(serialData[SERIAL_CONTROL_INDEX]);
  }

  if ((address >= TIMER_START) && (address <= TIMER_END)) {
    // Timer
    return timer_.read(address);
  }

  if (address == IF_REGISTER) {
    // Interrupt flags
    return interrupts_.intf() | IF_UNUSED_BITS;
  }

  if ((address >= APU_START) && (address <= APU_END)) {
    // Apu
    return apu_.read(address);
  }

  if ((address >= WAVE_RAM_START) && (address <= WAVE_RAM_END)) {
    // Apu
    return apu_.read(address);
  }

  if ((address >= LCD_START) && (address <= LCD_END)) {
    // Lcd
    return lcd_.read(address);
  }

  return 0;
}

void IO::write(uint16_t address, uint8_t value) {
  if (address == JOYPAD_REGISTER) {
    // Gamepad
    gamepad_.setSel(value);
    return;
  }

  if (address == SERIAL_DATA_REGISTER) {
    serialData[SERIAL_DATA_INDEX] = static_cast<char>(value);
    return;
  }

  if (address == SERIAL_CONTROL_REGISTER) {
    serialData[SERIAL_CONTROL_INDEX] = static_cast<char>(value);

    if ((value & BIT7_MASK) && serialSink) {
      serialSink(static_cast<uint8_t>(serialData[SERIAL_DATA_INDEX]));
      serialData[SERIAL_CONTROL_INDEX] = static_cast<char>(value & ~BIT7_MASK);
    }
    return;
  }

  if ((address >= TIMER_START) && (address <= TIMER_END)) {
    // Timer
    timer_.write(address, value);
    return;
  }

  if (address == IF_REGISTER) {
    // Interrupt flags
    interrupts_.setIntf(value);
    return;
  }

  if ((address >= APU_START) && (address <= APU_END)) {
    // Apu
    apu_.write(address, value);
    return;
  }

  if ((address >= WAVE_RAM_START) && (address <= WAVE_RAM_END)) {
    // Apu
    apu_.write(address, value);
    return;
  }

  if ((address >= LCD_START) && (address <= LCD_END)) {
    // Lcd
    lcd_.write(address, value);
    return;
  }
}