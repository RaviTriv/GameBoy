#include "../../../include/Channel.h"

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
  enabled = false;
  freqTimer = 0;
  lengthTimer = 0;
  frameTimer = 0;
  frameSequence = 0;

  duty = 0;
  envelopeVolume = 0;
  envelopeTimer = 0;
  envelopeEnabled = false;
}

bool SquareChannel::timerAction()
{
  return false;
}

void SquareChannel::frameSequencerAction()
{

}

bool SquareChannel::lengthTimerAction()
{
  return false;
}

uint8_t SquareChannel::getSample()
{
  return 0;
}

void SquareChannel::envelopeAction()
{
}

void SquareChannel::dutyAction()
{
}

void SquareChannel::sweepAction()
{
}