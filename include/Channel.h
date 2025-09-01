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
  int frameTimer = 0;
  int frameSequence = 0;
  uint16_t baseAddress = 0;
  friend class APU;

public:
  virtual ~Channel() = default;
  virtual void reset() = 0;
  virtual bool timerAction() = 0;
  virtual void frameSequencerAction() = 0;
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
  bool hasSweep = false;

  static const std::array<std::array<uint8_t, 8>, 4> duties;
  friend class APU;

public:
  SquareChannel();

  void reset() override;
  bool timerAction() override;
  bool lengthTimerAction() override;
  void frameSequencerAction() override;
  void envelopeAction();
  void dutyAction();
  uint8_t getSample() override;

  void sweepAction();
};

class WaveChannel : public Channel
{
private:
  friend class APU;
  uint8_t sample = 0;

public:
  void reset() override;
  bool timerAction() override;
  void frameSequencerAction() override;
  bool lengthTimerAction() override;
  uint8_t getSample() override;
  uint8_t getSample(uint8_t s);
};

class NoiseChannel : public Channel
{
private:
  friend class APU;
  int envelopeVolume;
  uint16_t lfsr;
  static const std::array<int, 8> divisor;

public:
  void reset() override;
  bool timerAction() override;
  void frameSequencerAction() override;
  bool lengthTimerAction() override;
  void envelopeAction();
  uint8_t getSample() override;
};