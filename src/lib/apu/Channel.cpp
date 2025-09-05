#include "../../../include/Channel.h"
#include "../../../include/Logger.h"

const std::array<std::array<uint8_t, 8>, 4> SquareChannel::duties = {{{0, 0, 0, 0, 0, 0, 0, 1},
                                                                      {1, 0, 0, 0, 0, 0, 0, 1},
                                                                      {1, 0, 0, 0, 0, 1, 1, 1},
                                                                      {0, 1, 1, 1, 1, 1, 1, 0}}};

void Channel::updateTriggers(bool lengthTrigger, bool envelopeTrigger, bool sweepTrigger)
{
  triggerLength = lengthTrigger;
  triggerEnvelope = envelopeTrigger;
  triggerSweep = sweepTrigger;
}

void SquareChannel::reset()
{
  nrx4 &= ~TRIGGER_BIT;

  envelopeVolume = (nrx2 & ENVELOPE_VOLUME_MASK) >> ENVELOPE_VOLUME_SHIFT;
  enabled = true;
  if ((nrx1 & LENGTH_MASK) != 0)
  {
    lengthTimer = MAX_LENGTH - (nrx1 & LENGTH_MASK);
  }
  envelopeEnabled = true;
}

bool SquareChannel::timerAction()
{
  if (freqTimer <= 0)
  {
    uint16_t wavelen = ((nrx4 & FREQ_HIGH_MASK) << FREQ_HIGH_SHIFT) | nrx3;
    freqTimer = TIMER_MULTIPLIER * (FREQ_BASE - wavelen);
    return true;
  }
  else
  {
    freqTimer--;
  }
  return false;
}

bool SquareChannel::lengthTimerAction()
{
  if (triggerLength && ((nrx4 & LENGTH_ENABLE_BIT) != 0) && lengthTimer)
  {
    lengthTimer--;
    if (lengthTimer <= 0)
    {
      return false;
    }
  }
  return true;
}

uint8_t SquareChannel::getSample()
{
  uint8_t sample = duties[((nrx1 & DUTY_MASK) >> DUTY_SHIFT)][duty];
  return sample * envelopeVolume * enabled;
}

void SquareChannel::envelopeAction()
{
  if (triggerEnvelope && envelopeEnabled && nrx2 & ENVELOPE_PERIOD_MASK)
  {
    envelopeTimer--;
    if (envelopeTimer <= 0)
    {
      envelopeTimer = nrx2 & ENVELOPE_PERIOD_MASK;
      int direction = (nrx2 & ENVELOPE_DIRECTION_BIT) ? 1 : -1;
      int newVolume = envelopeVolume + direction;
      if (newVolume >= 0 && newVolume <= MAX_VOLUME)
      {
        envelopeVolume = newVolume;
      }
      else
      {
        envelopeEnabled = false;
      }
    }
  }
}

void SquareChannel::dutyAction()
{
  duty++;
  duty %= DUTY_CYCLE_STEPS;
}

void WaveChannel::reset()
{
  nrx4 &= ~TRIGGER_BIT;
  enabled = true;
  sample = 0;
  if (!lengthTimer)
  {
    lengthTimer = MAX_LENGTH - nrx1;
  }
}

bool WaveChannel::timerAction()
{
  if (freqTimer <= 0)
  {
    uint16_t wavelen = ((nrx4 & FREQ_HIGH_MASK) << FREQ_HIGH_SHIFT) | nrx3;
    freqTimer = TIMER_MULTIPLIER * (FREQ_BASE - wavelen);
    return true;
  }
  else
  {
    freqTimer--;
  }
  return false;
}

bool WaveChannel::lengthTimerAction()
{
  if (triggerLength && ((nrx4 & LENGTH_ENABLE_BIT) != 0) && lengthTimer)
  {
    lengthTimer--;
    if (lengthTimer <= 0)
    {
      return false;
    }
  }
  return true;
}

uint8_t WaveChannel::getSample()
{
  return 0;
}

uint8_t WaveChannel::getSample(uint8_t s)
{
  return s * enabled * (nrx0 >>DAC_ENABLE_SHIFT);
}

const std::array<int, 8> NoiseChannel::divisor = {8, 16, 32, 48, 64, 80, 96, 112};

void NoiseChannel::reset()
{
  nrx4 &= ~TRIGGER_BIT;
  if (!lengthTimer)
  {
    lengthTimer = MAX_LENGTH - (nrx1 & LENGTH_MASK);
  }
  enabled = true;
  lfsr = INITIAL_LFSR;
  envelopeVolume = nrx2 >> ENVELOPE_VOLUME_SHIFT;
  envelopeEnabled = true;
}

bool NoiseChannel::lengthTimerAction()
{
  if (triggerLength && ((nrx4 & LENGTH_ENABLE_BIT) != 0) && triggerLength)
  {
    lengthTimer--;
    if (lengthTimer <= 0)
    {
      return false;
    }
  }
  return true;
}

void NoiseChannel::envelopeAction()
{
  if (triggerEnvelope && envelopeEnabled && nrx2 & ENVELOPE_PERIOD_MASK)
  {
    envelopeTimer--;
    if (envelopeTimer <= 0)
    {
      envelopeTimer = nrx2 & ENVELOPE_PERIOD_MASK;
      int direction = (nrx2 & ENVELOPE_DIRECTION_BIT) ? 1 : -1;
      int new_volume = envelopeVolume + direction;
      if (new_volume >= 0 && new_volume <= MAX_VOLUME)
      {
        envelopeVolume = new_volume;
      }
      else
      {
        envelopeEnabled = false;
      }
    }
  }
}

uint8_t NoiseChannel::getSample()
{
  return (~lfsr & LFSR_BIT0_MASK) * envelopeVolume * enabled;
}

bool NoiseChannel::timerAction()
{
  freqTimer--;

  if (freqTimer <= 0)
  {
    freqTimer = divisor[nrx3 & DIVISOR_INDEX_MASK] << (nrx3 >> SHIFT_AMOUNT_SHIFT);
    uint8_t xorRes = (lfsr & LFSR_BIT0_MASK) ^ ((lfsr & LFSR_BIT1_MASK) >> LFSR_BIT1_SHIFT);
    lfsr = (lfsr >> 1) | (xorRes << LFSR_FEEDBACK_BIT);

    if ((nrx3 >> LFSR_WIDTH_SHIFT) & LFSR_BIT0_MASK)
    {
      lfsr &= ~(1 << LFSR_7BIT_TAP);
      lfsr |= (xorRes << LFSR_7BIT_TAP);
    }
    return true;
  }
  return false;
}