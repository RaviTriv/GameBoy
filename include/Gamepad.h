#pragma once

#include <atomic>
#include <cstdint>

class Gamepad
{
public:
  bool isBPressed() const;
  bool isAPressed() const;
  bool isStartPressed() const;
  bool isSelectPressed() const;
  bool isUpPressed() const;
  bool isDownPressed() const;
  bool isLeftPressed() const;
  bool isRightPressed() const;

  void setBPressed(bool pressed);
  void setAPressed(bool pressed);
  void setStartPressed(bool pressed);
  void setSelectPressed(bool pressed);
  void setUpPressed(bool pressed);
  void setDownPressed(bool pressed);
  void setLeftPressed(bool pressed);
  void setRightPressed(bool pressed);

  bool actionSel() const;
  bool directionSel() const;
  void setSel(uint8_t value);
  uint8_t getOutput() const;

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
