#pragma once

#include "./Channel.h"

#include <array>
#include <cstdint>

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
  uint8_t read(uint16_t address);
  uint8_t getSample();

  static constexpr int audioFreq = 44100;

private:
  State state;
  static constexpr int SAMPLE_RATE = 95;
  uint16_t frameTimer = 0;
  uint8_t frameSequence = 0;
  bool triggerLength = false;
  bool triggerEnvelope = false;
  bool triggerSweep = false;

  void frameSequencerAction();

  uint8_t getChannel1Sample();
  uint8_t getChannel2Sample();
  uint8_t getChannel3Sample();
  uint8_t getChannel4Sample();

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
};