#pragma once

#include <array>
#include <cstdint>
#include <memory>

class APU;
class CPU;
class Timer;
class LCD;
class Gamepad;
class IO
{
public:
  IO(std::shared_ptr<CPU> cpu, std::shared_ptr<Timer> timer, std::shared_ptr<LCD> lcd, std::shared_ptr<Gamepad> gamepad, std::shared_ptr<APU> apu);
  void write(uint16_t address, uint8_t value);
  uint8_t read(uint16_t address) const;

  void setTimer(std::shared_ptr<Timer> timer);
  void setCPU(std::shared_ptr<CPU> cpu);
  void setLCD(std::shared_ptr<LCD> lcd);
  void setGamepad(std::shared_ptr<Gamepad> gamepad);
  void setApu(std::shared_ptr<APU> apu);

private:
  std::array<char, 2> serialData{};
  std::shared_ptr<APU> apu;
  std::shared_ptr<CPU> cpu;
  std::shared_ptr<Timer> timer;
  std::shared_ptr<LCD> lcd;
  std::shared_ptr<Gamepad> gamepad;
};