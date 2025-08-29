#pragma once

#include <cstdint>

class Gamepad
{
  struct State
  {
    bool start;
    bool select;
    bool a;
    bool b;
    bool up;
    bool down;
    bool left;
    bool right;
  };

public:
  bool isBPressed();
  bool isAPressed();
  bool isStartPressed();
  bool isSelectPressed();
  bool isUpPressed();
  bool isDownPressed();
  bool isLeftPressed();
  bool isRightPressed();

  void setBPressed(bool pressed);
  void setAPressed(bool pressed);
  void setStartPressed(bool pressed);
  void setSelectPressed(bool pressed);
  void setUpPressed(bool pressed);
  void setDownPressed(bool pressed);
  void setLeftPressed(bool pressed);
  void setRightPressed(bool pressed);

  bool actionSel();
  bool directionSel();
  void setSel(uint8_t value);
  uint8_t getOutput();

private:
  State state;
  bool directionSelected;
  bool actionSelected;
};