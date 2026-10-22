#include "Channel.h"

#include <array>
#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <utility>

#include "Serializable.h"

const std::array<std::array<uint8_t, 8>, 4> SquareChannel::duties = {
    {{0, 0, 0, 0, 0, 0, 0, 1},
     {1, 0, 0, 0, 0, 0, 0, 1},
     {1, 0, 0, 0, 0, 1, 1, 1},
     {0, 1, 1, 1, 1, 1, 1, 0}}};

bool Channel::writeControl(uint8_t value, int maxLength,
                           bool nextStepClocksLength) {
  const bool wasLengthEnabled = (nrx4 & LENGTH_ENABLE_BIT) != 0;
  const bool lengthEnabled = (value & LENGTH_ENABLE_BIT) != 0;
  const bool trigger = (value & TRIGGER_BIT) != 0;
  nrx4 = static_cast<uint8_t>(value & ~TRIGGER_BIT);

  if (!nextStepClocksLength && !wasLengthEnabled && lengthEnabled &&
      lengthTimer > 0) {
    lengthTimer--;
    if (lengthTimer == 0 && !trigger) {
      enabled = false;
    }
  }

  if (trigger && lengthTimer == 0) {
    lengthTimer = maxLength;
    if (lengthEnabled && !nextStepClocksLength) {
      lengthTimer--;
    }
  }
  return trigger;
}

void Channel::clockLength() {
  if ((nrx4 & LENGTH_ENABLE_BIT) != 0 && lengthTimer > 0) {
    lengthTimer--;
    if (lengthTimer == 0) {
      enabled = false;
    }
  }
}

bool Channel::envelopeDacEnabled() const {
  return (nrx2 & ENVELOPE_DAC_MASK) != 0;
}

void Channel::triggerEnvelope(bool nextStepClocksEnvelope) {
  envelopeVolume = (nrx2 & ENVELOPE_VOLUME_MASK) >> ENVELOPE_VOLUME_SHIFT;
  const int period = nrx2 & ENVELOPE_PERIOD_MASK;
  envelopeTimer = period != 0 ? period : ENVELOPE_PERIOD_ZERO;
  if (nextStepClocksEnvelope) {
    envelopeTimer++;
  }
  envelopeEnabled = true;
}

void SquareChannel::trigger(bool nextStepClocksEnvelope) {
  enabled = envelopeDacEnabled();
  freqTimer =
      TIMER_MULTIPLIER *
      (FREQ_BASE - (((nrx4 & FREQ_HIGH_MASK) << FREQ_HIGH_SHIFT) | nrx3));
  triggerEnvelope(nextStepClocksEnvelope);

  sweepShadowFreq = static_cast<uint16_t>(
      ((nrx4 & FREQ_HIGH_MASK) << FREQ_HIGH_SHIFT) | nrx3);
  uint8_t period = (nrx0 & SWEEP_PERIOD_MASK) >> SWEEP_PERIOD_SHIFT;
  uint8_t shift = nrx0 & SWEEP_SHIFT_MASK;
  sweepTimer = period ? period : 8;
  sweepEnabled = (period != 0) || (shift != 0);
  sweepNegateUsed = false;
  if (shift != 0 && sweepCalculation() >= SWEEP_OVERFLOW_THRESHOLD) {
    enabled = false;
  }
}

void SquareChannel::writeSweep(uint8_t value) {
  nrx0 = value;
  if ((value & SWEEP_NEGATE_BIT) == 0 && sweepNegateUsed) {
    enabled = false;
  }
}

uint8_t SquareChannel::getSample() const {
  uint8_t sample = duties[((nrx1 & DUTY_MASK) >> DUTY_SHIFT)][duty];
  return static_cast<uint8_t>(sample * envelopeVolume * enabled);
}

void Channel::envelopeAction() {
  if (envelopeEnabled && (nrx2 & ENVELOPE_PERIOD_MASK)) {
    envelopeTimer--;
    if (envelopeTimer <= 0) {
      envelopeTimer = nrx2 & ENVELOPE_PERIOD_MASK;
      int direction = (nrx2 & ENVELOPE_DIRECTION_BIT) ? 1 : -1;
      int newVolume = envelopeVolume + direction;
      if (newVolume >= 0 && std::cmp_less_equal(newVolume, MAX_VOLUME)) {
        envelopeVolume = newVolume;
      } else {
        envelopeEnabled = false;
      }
    }
  }
}

uint16_t SquareChannel::sweepCalculation() {
  uint8_t shift = nrx0 & SWEEP_SHIFT_MASK;
  auto delta = static_cast<uint16_t>(sweepShadowFreq >> shift);
  if (nrx0 & SWEEP_NEGATE_BIT) {
    sweepNegateUsed = true;
    return sweepShadowFreq - delta;
  }
  return sweepShadowFreq + delta;
}

void SquareChannel::sweepAction() {
  sweepTimer--;
  if (sweepTimer > 0) {
    return;
  }

  uint8_t period = (nrx0 & SWEEP_PERIOD_MASK) >> SWEEP_PERIOD_SHIFT;
  sweepTimer = period ? period : 8;
  if (!sweepEnabled || period == 0) {
    return;
  }

  uint16_t newFreq = sweepCalculation();
  if (newFreq >= SWEEP_OVERFLOW_THRESHOLD) {
    enabled = false;
    return;
  }

  uint8_t shift = nrx0 & SWEEP_SHIFT_MASK;
  if (shift != 0) {
    sweepShadowFreq = newFreq;
    nrx3 = static_cast<uint8_t>(newFreq & 0xFF);
    nrx4 =
        static_cast<uint8_t>((nrx4 & ~FREQ_HIGH_MASK) |
                             ((newFreq >> FREQ_HIGH_SHIFT) & FREQ_HIGH_MASK));

    uint16_t overflowCheck = sweepCalculation();
    if (overflowCheck >= SWEEP_OVERFLOW_THRESHOLD) {
      enabled = false;
    }
  }
}

int SquareChannel::advanceTimer(int ticks) {
  if (ticks <= 0) {
    return 0;
  }

  auto wavelen = static_cast<uint16_t>(
      ((nrx4 & FREQ_HIGH_MASK) << FREQ_HIGH_SHIFT) | nrx3);
  int period = TIMER_MULTIPLIER * (FREQ_BASE - wavelen);

  int fires = 0;

  // if timer already expired, first tick is a reload+fire
  if (freqTimer <= 0) {
    freqTimer = period;
    fires++;
    ticks--;
  }

  if (ticks <= 0) {
    return fires;
  }

  // fast path: no fires in remaining ticks
  if (ticks < freqTimer) {
    freqTimer -= ticks;
    return fires;
  }

  // consume current timer, then batch the rest
  ticks -= freqTimer;
  fires++;
  if (period > 0) {
    fires += ticks / period;
    freqTimer = period - (ticks % period);
  } else {
    fires += ticks;
    freqTimer = 0;
  }

  return fires;
}

int WaveChannel::period() const {
  return WAVE_TIMER_MULTIPLIER *
         (FREQ_BASE - (((nrx4 & FREQ_HIGH_MASK) << FREQ_HIGH_SHIFT) | nrx3));
}

bool WaveChannel::justFetched() const { return ticksSinceFetch < FETCH_WINDOW; }

std::size_t WaveChannel::currentByte() const { return sample / 2; }

void WaveChannel::trigger(std::array<uint8_t, 16>& waveRam) {
  if (enabled && freqTimer <= FETCH_WINDOW) {
    const std::size_t next = ((sample + 1U) / 2) % waveRam.size();
    if (next < CORRUPTION_BLOCK) {
      waveRam[0] = waveRam[next];
    } else {
      const std::size_t block = next & ~std::size_t{CORRUPTION_BLOCK - 1};
      for (std::size_t i = 0; i < CORRUPTION_BLOCK; i++) {
        waveRam[i] = waveRam[block + i];
      }
    }
  }
  enabled = (nrx0 & DAC_ENABLE_BIT) != 0;
  sample = 0;
  ticksSinceFetch = UINT8_MAX;
  freqTimer = period() + TRIGGER_DELAY;
}

uint8_t WaveChannel::getSample(uint8_t s) const {
  return static_cast<uint8_t>(s * enabled * (nrx0 >> DAC_ENABLE_SHIFT));
}

int WaveChannel::advanceTimer(int ticks) {
  int fires = 0;
  for (int i = 0; i < ticks; i++) {
    if (ticksSinceFetch < UINT8_MAX) {
      ticksSinceFetch++;
    }
    if (--freqTimer <= 0) {
      freqTimer = period();
      sample = static_cast<uint8_t>((sample + 1) % WAVE_SAMPLE_COUNT);
      ticksSinceFetch = 0;
      fires++;
    }
  }
  return fires;
}

const std::array<int, 8> NoiseChannel::divisor = {8,  16, 32, 48,
                                                  64, 80, 96, 112};

void NoiseChannel::trigger(bool nextStepClocksEnvelope) {
  enabled = envelopeDacEnabled();
  lfsr = INITIAL_LFSR;
  triggerEnvelope(nextStepClocksEnvelope);
}

uint8_t NoiseChannel::getSample() const {
  return static_cast<uint8_t>((~lfsr & LFSR_BIT0_MASK) * envelopeVolume *
                              enabled);
}

int NoiseChannel::advanceTimer(int ticks) {
  if (ticks <= 0) {
    return 0;
  }

  int period = divisor[nrx3 & DIVISOR_INDEX_MASK]
               << (nrx3 >> SHIFT_AMOUNT_SHIFT);
  int fires = 0;
  bool narrow = ((nrx3 >> LFSR_WIDTH_SHIFT) & LFSR_BIT0_MASK) != 0;

  // fast path: no fires in remaining ticks
  if (freqTimer > ticks) {
    freqTimer -= ticks;
    return 0;
  }

  // consume current timer to first fire
  ticks -= freqTimer;
  freqTimer = 0;

  // first fire
  fires++;
  {
    auto xorRes = static_cast<uint8_t>(
        (lfsr & LFSR_BIT0_MASK) ^ ((lfsr & LFSR_BIT1_MASK) >> LFSR_BIT1_SHIFT));
    lfsr = static_cast<uint16_t>((lfsr >> 1) | (xorRes << LFSR_FEEDBACK_BIT));
    if (narrow) {
      lfsr &= static_cast<uint16_t>(~(1 << LFSR_7BIT_TAP));
      lfsr |= static_cast<uint16_t>(xorRes << LFSR_7BIT_TAP);
    }
  }

  // batch remaining fires
  if (period > 0) {
    int remainingFires = ticks / period;
    freqTimer = period - (ticks % period);

    for (int i = 0; i < remainingFires; i++) {
      auto xorRes =
          static_cast<uint8_t>((lfsr & LFSR_BIT0_MASK) ^
                               ((lfsr & LFSR_BIT1_MASK) >> LFSR_BIT1_SHIFT));
      lfsr = static_cast<uint16_t>((lfsr >> 1) | (xorRes << LFSR_FEEDBACK_BIT));
      if (narrow) {
        lfsr &= static_cast<uint16_t>(~(1 << LFSR_7BIT_TAP));
        lfsr |= static_cast<uint16_t>(xorRes << LFSR_7BIT_TAP);
      }
    }
    fires += remainingFires;
  } else {
    freqTimer = 0;
  }

  return fires;
}
void SquareChannel::save(std::ostream& out) const {
  serial::saveFields(out, *this);
}

void SquareChannel::load(std::istream& in) {
  serial::loadFields(in, *this);
  if (duty >= duties[0].size()) {
    throw std::runtime_error("Invalid square channel duty in save file");
  }
}

void WaveChannel::save(std::ostream& out) const {
  serial::saveFields(out, *this);
}

void WaveChannel::load(std::istream& in) {
  serial::loadFields(in, *this);
  if (sample >= WAVE_SAMPLE_COUNT) {
    throw std::runtime_error("Invalid wave channel sample in save file");
  }
}

void NoiseChannel::save(std::ostream& out) const {
  serial::saveFields(out, *this);
}

void NoiseChannel::load(std::istream& in) { serial::loadFields(in, *this); }
