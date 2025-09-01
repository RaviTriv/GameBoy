#include "../../../include/Apu.h"
#include "../../../include/Channel.h"
#include "../../../include/Logger.h"

APU::APU()
{
  // state.channel1 = SquareChannel();
  //  state.channel2 = SquareChannel(0xFF15);
}

void APU::init()
{
}

uint8_t APU::read(uint16_t address)
{
  // TODO: Implement APU read
  switch (address)
  {
  case 0xFF10:
    return state.channel1.nrx0;
  case 0xFF11:
    return state.channel1.nrx1;
  case 0xFF12:
    return state.channel1.nrx2;
  case 0xFF13:
    return state.channel1.nrx3;
  case 0xFF14:
    return state.channel1.nrx4;
  case 0xFF16:
    return state.channel2.nrx1;
  case 0xFF17:
    return state.channel2.nrx2;
  case 0xFF18:
    return state.channel2.nrx3;
  case 0xFF19:
    return state.channel2.nrx4;
  case 0xFF1A:
    return state.channel3.nrx0;
  case 0xFF1B:
    return state.channel3.nrx1;
  case 0xFF1C:
    return state.channel3.nrx2;
  case 0xFF1D:
    return state.channel3.nrx3;
  case 0xFF1E:
    return state.channel3.nrx4;
  case 0xFF31:
    return state.wavePattern[1];
  case 0xFF32:
    return state.wavePattern[2];
  case 0xFF33:
    return state.wavePattern[3];
  case 0xFF34:
    return state.wavePattern[4];
  case 0xFF35:
    return state.wavePattern[5];
  case 0xFF36:
    return state.wavePattern[6];
  case 0xFF37:
    return state.wavePattern[7];
  case 0xFF38:
    return state.wavePattern[8];
  case 0xFF39:
    return state.wavePattern[9];
  case 0xFF3A:
    return state.wavePattern[10];
  case 0xFF3B:
    return state.wavePattern[11];
  case 0xFF3C:
    return state.wavePattern[12];
  case 0xFF3D:
    return state.wavePattern[13];
  case 0xFF3E:
    return state.wavePattern[14];
  case 0xFF3F:
    return state.wavePattern[15];
  default:
    // Logger::GetLogger()->info("APU READ");
    break;
  }
  return 0;
}

void APU::write(uint16_t address, uint8_t value)
{
  // TODO: Implement APU write
  switch (address)
  {
  case 0xFF10:
    state.channel1.nrx0 = value;
    break;
  case 0xFF11:
    state.channel1.nrx1 = value;
    break;
  case 0xFF12:
    state.channel1.nrx2 = value;
    break;
  case 0xFF13:
    state.channel1.nrx3 = value;
    break;
  case 0xFF14:
    state.channel1.nrx4 = value;
    break;
  case 0xFF16:
    state.channel2.nrx1 = value;
    break;
  case 0xFF17:
    state.channel2.nrx2 = value;
    break;
  case 0xFF18:
    state.channel2.nrx3 = value;
    break;
  case 0xFF19:
    state.channel2.nrx4 = value;
    break;
  case 0xFF1A:
    state.channel3.nrx0 = value;
    break;
  case 0xFF1B:
    state.channel3.nrx1 = value;
    break;
  case 0xFF1C:
    state.channel3.nrx2 = value;
    break;
  case 0xFF1D:
    state.channel3.nrx3 = value;
    break;
  case 0xFF1E:
    state.channel3.nrx4 = value;
    break;
  case 0xFF30:
    state.wavePattern[0] = value;
    break;
  case 0xFF31:
    state.wavePattern[1] = value;
    break;
  case 0xFF32:
    state.wavePattern[2] = value;
    break;
  case 0xFF33:
    state.wavePattern[3] = value;
    break;
  case 0xFF34:
    state.wavePattern[4] = value;
    break;
  case 0xFF35:
    state.wavePattern[5] = value;
    break;
  case 0xFF36:
    state.wavePattern[6] = value;
    break;
  case 0xFF37:
    state.wavePattern[7] = value;
    break;
  case 0xFF38:
    state.wavePattern[8] = value;
    break;
  case 0xFF39:
    state.wavePattern[9] = value;
    break;
  case 0xFF3A:
    state.wavePattern[10] = value;
    break;
  case 0xFF3B:
    state.wavePattern[11] = value;
    break;
  case 0xFF3C:
    state.wavePattern[12] = value;
    break;
  case 0xFF3D:
    state.wavePattern[13] = value;
    break;
  case 0xFF3E:
    state.wavePattern[14] = value;
    break;
  case 0xFF3F:
    state.wavePattern[15] = value;
    break;
  default:
    // Logger::GetLogger()->info("APU WRITE");
    break;
  };
}

uint8_t APU::getChannel1Sample()
{
  if ((state.channel1.nrx4 & 0x80) != 0)
  {
    state.channel1.reset();
  }

  bool timerTriggered = state.channel1.timerAction();

  if (timerTriggered)
  {
    state.channel1.dutyAction();
  }

  state.channel1.frameSequencerAction();

  state.channel1.enabled &= state.channel1.lengthTimerAction();

  if (state.channel1.envelopeEnabled)
  {
    state.channel1.envelopeAction();
  }

  return state.channel1.getSample();
}

uint8_t APU::getChannel2Sample()
{
  if ((state.channel2.nrx4 & 0x80) != 0)
  {
    state.channel2.reset();
  }

  bool timerTriggered = state.channel2.timerAction();

  if (timerTriggered)
  {
    state.channel2.dutyAction();
  }

  state.channel2.frameSequencerAction();

  state.channel2.enabled &= state.channel2.lengthTimerAction();

  if (state.channel2.envelopeEnabled)
  {
    state.channel2.envelopeAction();
  }

  return state.channel2.getSample();
}

uint8_t APU::getChannel3Sample()
{
  if ((state.channel3.nrx4 & 0x80) != 0)
  {
    state.channel3.reset();
  }

  state.channel3.frameSequencerAction();

  bool timerTriggered = state.channel3.timerAction();

  if (timerTriggered)
  {
    ++state.channel3.sample %= 32;
  }

  uint8_t sample = state.wavePattern[state.channel3.sample / 2];

  if (state.channel3.sample % 2)
  {
    sample = sample & 0x0F;
  }
  else
  {
    sample = sample >> 4;
  }

  state.channel3.enabled &= state.channel3.lengthTimerAction();

  int shiftVol = ((state.channel3.nrx2 >> 5) & 0x03) ? ((state.channel3.nrx2 >> 5) & 0x03) - 1 : 4;

  sample >>= shiftVol;

  return state.channel3.getSample(sample);
}

uint8_t APU::getChannel4Sample()
{
  if ((state.channel4.nrx4 & 0x80) != 0)
  {
    state.channel4.reset();
  }

  state.channel4.frameSequencerAction();

  state.channel4.freqTimer--;

  if (state.channel4.freqTimer <= 0)
  {
    state.channel4.freqTimer = NoiseChannel::divisor[state.channel4.nrx3 & 0x07] << (state.channel4.nrx3 >> 4);
    uint8_t xorRes = (state.channel4.lfsr & 0x01) ^ ((state.channel4.lfsr & 0x02) >> 1);
    state.channel4.lfsr = (state.channel4.lfsr >> 1) | (xorRes << 14);

    if ((state.channel4.nrx3 >> 3) & 0x01)
    {
      state.channel4.lfsr &= ~(1 << 6);
      state.channel4.lfsr |= (xorRes << 6);
    }
  }

  state.channel4.enabled &= state.channel4.lengthTimerAction();

  state.channel4.envelopeAction();

  uint8_t sample = (~state.channel4.lfsr & 0x01) * state.channel4.envelopeVolume;
  return sample * state.channel4.enabled;
}

uint8_t APU::getSample()
{
  uint8_t ch1Sample = 0;
  uint8_t ch2Sample = 0;
  uint8_t ch3Sample = 0;
  uint8_t ch4Sample = 0;

  for (int i = 0; i < SAMPLE_RATE; i++)
  {
    ch1Sample = getChannel1Sample();
    ch2Sample = getChannel2Sample();
    ch3Sample = getChannel3Sample();
    ch4Sample = getChannel4Sample();
  }

  return ch1Sample + ch2Sample + ch3Sample + ch4Sample;
}