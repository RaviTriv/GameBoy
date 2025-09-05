#include "../../include/Gamepad.h"

bool Gamepad::isBPressed()
{
  return state.b;
}
bool Gamepad::isAPressed()
{
  return state.a;
}
bool Gamepad::isStartPressed()
{
  return state.start;
}
bool Gamepad::isSelectPressed()
{
  return state.select;
}
bool Gamepad::isUpPressed()
{
  return state.up;
}
bool Gamepad::isDownPressed()
{
  return state.down;
}
bool Gamepad::isLeftPressed()
{
  return state.left;
}
bool Gamepad::isRightPressed()
{
  return state.right;
};

void Gamepad::setBPressed(bool pressed)
{
  state.b = pressed;
}
void Gamepad::setAPressed(bool pressed)
{
  state.a = pressed;
}
void Gamepad::setStartPressed(bool pressed)
{
  state.start = pressed;
}
void Gamepad::setSelectPressed(bool pressed)
{
  state.select = pressed;
}
void Gamepad::setUpPressed(bool pressed)
{
  state.up = pressed;
}
void Gamepad::setDownPressed(bool pressed)
{
  state.down = pressed;
}
void Gamepad::setLeftPressed(bool pressed)
{
  state.left = pressed;
}
void Gamepad::setRightPressed(bool pressed)
{
  state.right = pressed;
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

  if (!actionSel())
  {
    if (state.start)
    {
      output &= ~(1 << 3);
    }
    if (state.select)
    {
      output &= ~(1 << 2);
    }
    if (state.a)
    {
      output &= ~(1 << 0);
    }
    if (state.b)
    {
      output &= ~(1 << 1);
    }
  }

  if (!directionSel())
  {
    if (state.left)
    {
      output &= ~(1 << 1);
    }
    if (state.right)
    {
      output &= ~(1 << 0);
    }
    if (state.up)
    {
      output &= ~(1 << 2);
    }
    if (state.down)
    {
      output &= ~(1 << 3);
    }
  }

  return output;
}