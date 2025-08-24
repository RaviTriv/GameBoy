#pragma once

#include <array>
#include <cstdint>
class CPU;
class Timer;
class LCD;
class IO
{
public:
  IO(std::shared_ptr<CPU> cpu, std::shared_ptr<Timer> timer, std::shared_ptr<LCD> lcd);
  void write(uint16_t address, uint8_t value);
  uint8_t read(uint16_t address) const;

  void setTimer(std::shared_ptr<Timer> timer);
  void setCPU(std::shared_ptr<CPU> cpu);
  void setLCD(std::shared_ptr<LCD> lcd);
private:
  std::array<char, 2> serialData{};
  std::shared_ptr<CPU> cpu;
  std::shared_ptr<Timer> timer;
  std::shared_ptr<LCD> lcd;
};