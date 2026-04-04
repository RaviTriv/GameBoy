#pragma once

#include "./Channel.h"
#include "spsc_queue.hpp"

#include <array>
#include <cstdint>

struct StereoSample
{
  uint8_t left;
  uint8_t right;
};

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
    SquareChannel channel2;
    WaveChannel channel3;
    NoiseChannel channel4;
    std::array<uint8_t, 16> wavePattern;
    bool enabled;
    uint32_t sampleRate;
    uint32_t cycleCounter;
  };

public:
  void write(uint16_t address, uint8_t value);
  [[nodiscard]] uint8_t read(uint16_t address) const;

  void tick();

  std::size_t popSamples(StereoSample *out, std::size_t count);

  static constexpr int audioFreq = 44100;

private:
  State state;
  spsc::Queue<StereoSample, 8192> sampleQueue;
  static constexpr int SAMPLE_RATE = 95;
  uint16_t frameTimer = 0;
  uint8_t frameSequence = 0;
  bool triggerLength = false;
  bool triggerEnvelope = false;
  bool triggerSweep = false;
  uint32_t sampleTimer = 0;
  uint32_t pendingTicks = 0;
  static constexpr uint32_t CPU_CLOCK_HZ = 4194304;
  static constexpr uint32_t CPU_CYCLES_PER_SAMPLE = CPU_CLOCK_HZ / audioFreq;

  void flushChannelTimers();
  [[nodiscard]] StereoSample mixSample();

  [[nodiscard]] uint8_t getChannel3CurrentSample();

  static constexpr uint16_t NR10_REGISTER = 0xFF10;
  static constexpr uint16_t NR11_REGISTER = 0xFF11;
  static constexpr uint16_t NR12_REGISTER = 0xFF12;
  static constexpr uint16_t NR13_REGISTER = 0xFF13;
  static constexpr uint16_t NR14_REGISTER = 0xFF14;
  static constexpr uint16_t NR21_REGISTER = 0xFF16;
  static constexpr uint16_t NR22_REGISTER = 0xFF17;
  static constexpr uint16_t NR23_REGISTER = 0xFF18;
  static constexpr uint16_t NR24_REGISTER = 0xFF19;
  static constexpr uint16_t NR30_REGISTER = 0xFF1A;
  static constexpr uint16_t NR31_REGISTER = 0xFF1B;
  static constexpr uint16_t NR32_REGISTER = 0xFF1C;
  static constexpr uint16_t NR33_REGISTER = 0xFF1D;
  static constexpr uint16_t NR34_REGISTER = 0xFF1E;
  static constexpr uint16_t NR41_REGISTER = 0xFF20;
  static constexpr uint16_t NR42_REGISTER = 0xFF21;
  static constexpr uint16_t NR43_REGISTER = 0xFF22;
  static constexpr uint16_t NR44_REGISTER = 0xFF23;
  static constexpr uint16_t NR50_REGISTER = 0xFF24;
  static constexpr uint16_t NR51_REGISTER = 0xFF25;
  static constexpr uint16_t NR52_REGISTER = 0xFF26;
  static constexpr uint16_t WAVE_RAM_START = 0xFF30;
  static constexpr uint16_t WAVE_RAM_END = 0xFF3F;
  static constexpr uint8_t TRIGGER_BIT = 0x80;
  static constexpr uint8_t ENABLE_BIT = 0x80;
  static constexpr uint8_t LENGTH_ENABLE_BIT = 0x40;
  static constexpr uint8_t WAVE_VOLUME_SHIFT_MASK = 0x60;
  static constexpr uint8_t WAVE_VOLUME_SHIFT_POS = 5;
  static constexpr uint8_t WAVE_VOLUME_0_SHIFT = 4;
  static constexpr uint8_t WAVE_HIGH_NIBBLE_MASK = 0xF0;
  static constexpr uint8_t WAVE_LOW_NIBBLE_MASK = 0x0F;
  static constexpr uint8_t NIBBLE_SIZE = 4;
  static constexpr uint16_t FRAME_SEQUENCER_CLOCK = 8192;
  static constexpr uint8_t FRAME_SEQUENCER_STEPS = 8;
  static constexpr uint8_t WAVE_SAMPLE_COUNT = 32;

  static constexpr uint8_t NR51_CH1_RIGHT = 0x01;
  static constexpr uint8_t NR51_CH2_RIGHT = 0x02;
  static constexpr uint8_t NR51_CH3_RIGHT = 0x04;
  static constexpr uint8_t NR51_CH4_RIGHT = 0x08;
  static constexpr uint8_t NR51_CH1_LEFT = 0x10;
  static constexpr uint8_t NR51_CH2_LEFT = 0x20;
  static constexpr uint8_t NR51_CH3_LEFT = 0x40;
  static constexpr uint8_t NR51_CH4_LEFT = 0x80;

  static constexpr uint8_t NR50_VOLUME_MASK = 0x07;
  static constexpr uint8_t NR50_LEFT_VOLUME_SHIFT = 4;
};
