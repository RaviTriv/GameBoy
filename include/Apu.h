#pragma once

#include "./Channel.h"

#include <array>
#include <cstdint>

class Bus;
class APU
{
  struct Registers
  {
    uint8_t NR52;
    uint8_t NR51;
    uint8_t NR50;
  };

  struct State
  {
    Registers registers;
    SquareChannel channel1;
    std::array<uint8_t, 16> wavePattern;
    bool enabled;
    uint32_t sampleRate;
    uint32_t cycleCounter;
  };

public:
  APU(std::shared_ptr<Bus> bus);
  void setBus(std::shared_ptr<Bus> bus);

  void init();

  void write(uint16_t address, uint8_t value);
  uint8_t read(uint16_t address);
  uint8_t getSample();

  static constexpr int audioFreq = 44100;

private:
  State state;
  std::shared_ptr<Bus> bus;
  static constexpr int SAMPLE_RATE = 95;
  uint8_t getChannel1Sample();
};