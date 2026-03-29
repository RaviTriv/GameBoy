#pragma once

#include <atomic>
#include <cstdint>

class Gamepad
{
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
  std::atomic<uint8_t> buttons{0};
  bool directionSelected;
  bool actionSelected;

  static constexpr uint8_t BTN_A = 1 << 0;
  static constexpr uint8_t BTN_B = 1 << 1;
  static constexpr uint8_t BTN_SELECT = 1 << 2;
  static constexpr uint8_t BTN_START = 1 << 3;
  static constexpr uint8_t BTN_UP = 1 << 4;
  static constexpr uint8_t BTN_DOWN = 1 << 5;
  static constexpr uint8_t BTN_LEFT = 1 << 6;
  static constexpr uint8_t BTN_RIGHT = 1 << 7;

  static constexpr uint8_t ACTION_SELECT_BIT = 0x20;
  static constexpr uint8_t DIRECTION_SELECT_BIT = 0x10;
  static constexpr uint8_t DEFAULT_OUTPUT = 0xCF;
};
