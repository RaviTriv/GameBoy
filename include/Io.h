#pragma once

#include <array>
#include <cstdint>

class IO
{
public:
  void write(uint16_t address, uint8_t value);
  uint8_t read(uint16_t address) const;

private:
  std::array<char, 2> serialData{};
};