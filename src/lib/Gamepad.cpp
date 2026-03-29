#include "../../include/Gamepad.h"

bool Gamepad::isBPressed()
{
  return buttons.load(std::memory_order_relaxed) & BTN_B;
}
bool Gamepad::isAPressed()
{
  return buttons.load(std::memory_order_relaxed) & BTN_A;
}
bool Gamepad::isStartPressed()
{
  return buttons.load(std::memory_order_relaxed) & BTN_START;
}
bool Gamepad::isSelectPressed()
{
  return buttons.load(std::memory_order_relaxed) & BTN_SELECT;
}
bool Gamepad::isUpPressed()
{
  return buttons.load(std::memory_order_relaxed) & BTN_UP;
}
bool Gamepad::isDownPressed()
{
  return buttons.load(std::memory_order_relaxed) & BTN_DOWN;
}
bool Gamepad::isLeftPressed()
{
  return buttons.load(std::memory_order_relaxed) & BTN_LEFT;
}
bool Gamepad::isRightPressed()
{
  return buttons.load(std::memory_order_relaxed) & BTN_RIGHT;
}

void Gamepad::setBPressed(bool pressed)
{
  if (pressed)
    buttons.fetch_or(BTN_B, std::memory_order_relaxed);
  else
    buttons.fetch_and(static_cast<uint8_t>(~BTN_B), std::memory_order_relaxed);
}
void Gamepad::setAPressed(bool pressed)
{
  if (pressed)
    buttons.fetch_or(BTN_A, std::memory_order_relaxed);
  else
    buttons.fetch_and(static_cast<uint8_t>(~BTN_A), std::memory_order_relaxed);
}
void Gamepad::setStartPressed(bool pressed)
{
  if (pressed)
    buttons.fetch_or(BTN_START, std::memory_order_relaxed);
  else
    buttons.fetch_and(static_cast<uint8_t>(~BTN_START), std::memory_order_relaxed);
}
void Gamepad::setSelectPressed(bool pressed)
{
  if (pressed)
    buttons.fetch_or(BTN_SELECT, std::memory_order_relaxed);
  else
    buttons.fetch_and(static_cast<uint8_t>(~BTN_SELECT), std::memory_order_relaxed);
}
void Gamepad::setUpPressed(bool pressed)
{
  if (pressed)
    buttons.fetch_or(BTN_UP, std::memory_order_relaxed);
  else
    buttons.fetch_and(static_cast<uint8_t>(~BTN_UP), std::memory_order_relaxed);
}
void Gamepad::setDownPressed(bool pressed)
{
  if (pressed)
    buttons.fetch_or(BTN_DOWN, std::memory_order_relaxed);
  else
    buttons.fetch_and(static_cast<uint8_t>(~BTN_DOWN), std::memory_order_relaxed);
}
void Gamepad::setLeftPressed(bool pressed)
{
  if (pressed)
    buttons.fetch_or(BTN_LEFT, std::memory_order_relaxed);
  else
    buttons.fetch_and(static_cast<uint8_t>(~BTN_LEFT), std::memory_order_relaxed);
}
void Gamepad::setRightPressed(bool pressed)
{
  if (pressed)
    buttons.fetch_or(BTN_RIGHT, std::memory_order_relaxed);
  else
    buttons.fetch_and(static_cast<uint8_t>(~BTN_RIGHT), std::memory_order_relaxed);
}

bool Gamepad::actionSel()
{
  return actionSelected;
}

bool Gamepad::directionSel()
{
  return directionSelected;
}

void Gamepad::setSel(uint8_t value)
{
  actionSelected = value & ACTION_SELECT_BIT;
  directionSelected = value & DIRECTION_SELECT_BIT;
}

uint8_t Gamepad::getOutput()
{
  uint8_t output = DEFAULT_OUTPUT;
  uint8_t btns = buttons.load(std::memory_order_relaxed);

  if (!actionSel())
  {
    if (btns & BTN_START)
    {
      output &= ~(1 << 3);
    }
    if (btns & BTN_SELECT)
    {
      output &= ~(1 << 2);
    }
    if (btns & BTN_A)
    {
      output &= ~(1 << 0);
    }
    if (btns & BTN_B)
    {
      output &= ~(1 << 1);
    }
  }

  if (!directionSel())
  {
    if (btns & BTN_LEFT)
    {
      output &= ~(1 << 1);
    }
    if (btns & BTN_RIGHT)
    {
      output &= ~(1 << 0);
    }
    if (btns & BTN_UP)
    {
      output &= ~(1 << 2);
    }
    if (btns & BTN_DOWN)
    {
      output &= ~(1 << 3);
    }
  }

  return output;
}
