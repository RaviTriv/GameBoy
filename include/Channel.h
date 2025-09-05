#pragma once

#include <array>
#include <cstdint>
#include <memory>

class APU;
class Bus;
class Channel
{
protected:
  uint8_t nrx0, nrx1, nrx2, nrx3, nrx4;
  bool enabled = false;
  bool triggerLength = false;
  bool triggerEnvelope = false;
  bool triggerSweep = false;
  int freqTimer = 0;
  int lengthTimer = 0;
  uint16_t baseAddress = 0;
  friend class APU;

  static constexpr uint8_t TRIGGER_BIT = 0x80;
  static constexpr uint8_t LENGTH_ENABLE_BIT = 0x40;
  static constexpr uint16_t FREQ_HIGH_MASK = 0x07;
  static constexpr uint8_t FREQ_HIGH_SHIFT = 8;
  static constexpr uint16_t FREQ_BASE = 2048;
  static constexpr uint8_t TIMER_MULTIPLIER = 4;
  static constexpr uint8_t ENVELOPE_VOLUME_SHIFT = 4;
  static constexpr uint8_t ENVELOPE_PERIOD_MASK = 0x07;
  static constexpr uint8_t ENVELOPE_VOLUME_MASK = 0xF0;
  static constexpr uint8_t ENVELOPE_DIRECTION_BIT = 0x08;
  static constexpr uint8_t MAX_VOLUME = 0x0F;

public:
  virtual ~Channel() = default;
  void updateTriggers(bool lengthTrigger, bool envelopeTrigger, bool sweepTrigger);
  virtual void reset() = 0;
  virtual bool timerAction() = 0;
  virtual bool lengthTimerAction() = 0;
  virtual uint8_t getSample() = 0;
};

class SquareChannel : public Channel
{
private:
  uint8_t duty = 0;
  int envelopeVolume = 0;
  int envelopeTimer = 0;
  bool envelopeEnabled = false;

  static const std::array<std::array<uint8_t, 8>, 4> duties;
  friend class APU;
  static constexpr uint8_t DUTY_MASK = 0xC0;     
  static constexpr uint8_t DUTY_SHIFT = 6;      
  static constexpr uint8_t LENGTH_MASK = 0x3F;   
  static constexpr uint8_t MAX_LENGTH = 64;     
  static constexpr uint8_t DUTY_CYCLE_STEPS = 8; 
public:
  void reset() override;
  bool timerAction() override;
  bool lengthTimerAction() override;
  void envelopeAction();
  void dutyAction();
  uint8_t getSample() override;
};

class WaveChannel : public Channel
{
private:
  friend class APU;
  uint8_t sample = 0;
  static constexpr uint16_t MAX_LENGTH = 256;     
  static constexpr uint8_t DAC_ENABLE_BIT = 0x80; 
  static constexpr uint8_t DAC_ENABLE_SHIFT = 7;  
  static constexpr uint8_t WAVE_SAMPLE_COUNT = 32; 
public:
  void reset() override;
  bool timerAction() override;
  bool lengthTimerAction() override;
  uint8_t getSample() override;
  uint8_t getSample(uint8_t s);
};

class NoiseChannel : public Channel
{
private:
  friend class APU;
  bool envelopeEnabled;
  int envelopeVolume;
  int envelopeTimer;
  uint16_t lfsr;
  static const std::array<int, 8> divisor;
  static constexpr uint8_t LENGTH_MASK = 0x3F;     
  static constexpr uint8_t MAX_LENGTH = 64;     
  static constexpr uint16_t INITIAL_LFSR = 0x7FFF;

  static constexpr uint8_t DIVISOR_INDEX_MASK = 0x07;
  static constexpr uint8_t SHIFT_AMOUNT_SHIFT = 4;   

  static constexpr uint8_t LFSR_BIT0_MASK = 0x01;  
  static constexpr uint8_t LFSR_BIT1_MASK = 0x02;  
  static constexpr uint8_t LFSR_BIT1_SHIFT = 1;    
  static constexpr uint8_t LFSR_WIDTH_BIT = 0x08; 
  static constexpr uint8_t LFSR_WIDTH_SHIFT = 3;  
  static constexpr uint8_t LFSR_FEEDBACK_BIT = 14;
  static constexpr uint8_t LFSR_7BIT_TAP = 6;     
public:
  void reset() override;
  bool timerAction() override;
  bool lengthTimerAction() override;
  void envelopeAction();
  uint8_t getSample() override;
};