#include "../../../include/Channel.h"
#include "../../../include/Bus.h"
#include "../../../include/Logger.h"

const std::array<std::array<uint8_t, 8>, 4> SquareChannel::duties = {{{0, 0, 0, 0, 0, 0, 0, 1},
                                                                      {1, 0, 0, 0, 0, 0, 0, 1},
                                                                      {1, 0, 0, 0, 0, 1, 1, 1},
                                                                      {0, 1, 1, 1, 1, 1, 1, 0}}};

SquareChannel::SquareChannel()
{
  duty = 0;
  envelopeVolume = 0;
  envelopeTimer = 0;
  envelopeEnabled = false;
  hasSweep = false;
}

void SquareChannel::reset()
{
  nrx4 &= ~0x80;

  envelopeVolume = (nrx2 & 0xF0) >> 4;
  enabled = true;
  if ((nrx1 & 0x3F) != 0)
  {
    lengthTimer = 64 - (nrx1 & 0x3F);
  }
  envelopeEnabled = true;
}

bool SquareChannel::timerAction()
{
  if (freqTimer <= 0)
  {
    uint16_t wavelen = ((nrx4 & 0x07) << 8) | nrx3;
    freqTimer = 4 * (2048 - wavelen);
    return true;
  }
  else
  {
    freqTimer--;
  }
  return false;
}

void SquareChannel::frameSequencerAction()
{
  frameTimer++;
  if (frameTimer == 8192)
  {
    frameTimer = 0;
    frameSequence++;
    frameSequence %= 8;

    triggerLength = frameSequence % 2 == 0;
    triggerEnvelope = frameSequence == 7;
    triggerSweep = frameSequence == 2 || frameSequence == 6;
  }
  else
  {
    triggerLength = false;
    triggerEnvelope = false;
    triggerSweep = false;
  }
}

bool SquareChannel::lengthTimerAction()
{
  if (triggerLength && ((nrx4 & 0x40) != 0) && lengthTimer)
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
  uint8_t sample = duties[((nrx1 & 0xC0) >> 6)][duty];
  return sample * envelopeVolume * enabled;
}

void SquareChannel::envelopeAction()
{
  if (triggerEnvelope && envelopeEnabled && nrx2 & 0x07)
  {
    envelopeTimer--;
    if (envelopeTimer <= 0)
    {
      envelopeTimer = nrx2 & 0x07;
      int direction = (nrx2 & 0x08) ? 1 : -1;
      int newVolume = envelopeVolume + direction;
      if (newVolume >= 0 && newVolume <= 0x0F)
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
  duty %= 8;
}

void SquareChannel::sweepAction()
{
}