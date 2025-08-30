#pragma once

#include <array>
#include <cstdint>

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
  friend class APU;

public:
  virtual ~Channel() = default;
  virtual void reset() = 0;
  virtual bool timerAction() = 0;
  virtual void frameSequencerAction() = 0;
  virtual bool lengthTimerAction() = 0;
  virtual uint8_t getSample() = 0;

  void write(uint8_t reg, uint8_t value);
  uint8_t read(uint8_t reg);
};

class SquareChannel : public Channel
{
private:
  uint8_t NRx0 = 0;
  uint8_t NRx1 = 0;
  uint8_t NRx2 = 0;
  uint8_t NRx3 = 0;
  uint8_t NRx4 = 0;

  uint8_t duty = 0;
  int envelopeVolume = 0;
  int envelopeTimer = 0;
  bool envelopeEnabled = false;
  bool hasSweep = false;

  static const std::array<std::array<uint8_t, 8>, 4> duties;

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