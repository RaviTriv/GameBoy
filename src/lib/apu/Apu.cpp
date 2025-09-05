#include "../../../include/Apu.h"
#include "../../../include/Channel.h"
#include "../../../include/Logger.h"

uint8_t APU::read(uint16_t address)
{
  switch (address)
  {
  case NR10_REGISTER:
    return state.channel1.nrx0;
  case NR11_REGISTER:
    return state.channel1.nrx1;
  case NR12_REGISTER:
    return state.channel1.nrx2;
  case NR13_REGISTER:
    return state.channel1.nrx3;
  case NR14_REGISTER:
    return state.channel1.nrx4;
  case NR21_REGISTER:
    return state.channel2.nrx1;
  case NR22_REGISTER:
    return state.channel2.nrx2;
  case NR23_REGISTER:
    return state.channel2.nrx3;
  case NR24_REGISTER:
    return state.channel2.nrx4;
  case NR30_REGISTER:
    return state.channel3.nrx0;
  case NR31_REGISTER:
    return state.channel3.nrx1;
  case NR32_REGISTER:
    return state.channel3.nrx2;
  case NR33_REGISTER:
    return state.channel3.nrx3;
  case NR34_REGISTER:
    return state.channel3.nrx4;
  case NR41_REGISTER:
    return state.channel4.nrx1;
  case NR42_REGISTER:
    return state.channel4.nrx2;
  case NR43_REGISTER:
    return state.channel4.nrx3;
  case NR44_REGISTER:
    return state.channel4.nrx4;
  case NR50_REGISTER:
    return state.registers.NR50;
  case NR51_REGISTER:
    return state.registers.NR51;
  case NR52_REGISTER:
    return state.registers.NR52 | (state.enabled ? ENABLE_BIT : 0x00);
  case WAVE_RAM_START + 1:
    return state.wavePattern[1];
  case WAVE_RAM_START + 2:
    return state.wavePattern[2];
  case WAVE_RAM_START + 3:
    return state.wavePattern[3];
  case WAVE_RAM_START + 4:
    return state.wavePattern[4];
  case WAVE_RAM_START + 5:
    return state.wavePattern[5];
  case WAVE_RAM_START + 6:
    return state.wavePattern[6];
  case WAVE_RAM_START + 7:
    return state.wavePattern[7];
  case WAVE_RAM_START + 8:
    return state.wavePattern[8];
  case WAVE_RAM_START + 9:
    return state.wavePattern[9];
  case WAVE_RAM_START + 10:
    return state.wavePattern[10];
  case WAVE_RAM_START + 11:
    return state.wavePattern[11];
  case WAVE_RAM_START + 12:
    return state.wavePattern[12];
  case WAVE_RAM_START + 13:
    return state.wavePattern[13];
  case WAVE_RAM_START + 14:
    return state.wavePattern[14];
  case WAVE_RAM_START + 15:
    return state.wavePattern[15];
  default:
    throw std::runtime_error("Invalid APU read");
    break;
  }
  return 0;
}

void APU::write(uint16_t address, uint8_t value)
{
  switch (address)
  {
  case NR10_REGISTER:
    state.channel1.nrx0 = value;
    break;
  case NR11_REGISTER:
    state.channel1.nrx1 = value;
    break;
  case NR12_REGISTER:
    state.channel1.nrx2 = value;
    break;
  case NR13_REGISTER:
    state.channel1.nrx3 = value;
    break;
  case NR14_REGISTER:
    state.channel1.nrx4 = value;
    break;
  case NR21_REGISTER:
    state.channel2.nrx1 = value;
    break;
  case NR22_REGISTER:
    state.channel2.nrx2 = value;
    break;
  case NR23_REGISTER:
    state.channel2.nrx3 = value;
    break;
  case NR24_REGISTER:
    state.channel2.nrx4 = value;
    break;
  case NR30_REGISTER:
    state.channel3.nrx0 = value;
    break;
  case NR31_REGISTER:
    state.channel3.nrx1 = value;
    break;
  case NR32_REGISTER:
    state.channel3.nrx2 = value;
    break;
  case NR33_REGISTER:
    state.channel3.nrx3 = value;
    break;
  case NR34_REGISTER:
    state.channel3.nrx4 = value;
    break;
  case NR41_REGISTER:
    state.channel4.nrx1 = value;
    break;
  case NR42_REGISTER:
    state.channel4.nrx2 = value;
    break;
  case NR43_REGISTER:
    state.channel4.nrx3 = value;
    break;
  case NR44_REGISTER:
    state.channel4.nrx4 = value;
    break;
  case NR50_REGISTER:
    state.registers.NR50 = value;
    break;
  case NR51_REGISTER:
    state.registers.NR51 = value;
    break;
  case NR52_REGISTER:
    state.registers.NR52 = value;
    state.enabled = (value & ENABLE_BIT) != 0;
    if (!state.enabled)
    {
      state.channel1 = {};
      state.channel2 = {};
      state.channel3 = {};
      state.channel4 = {};
      state.wavePattern.fill(0);
    }
    break;
  case WAVE_RAM_START:
    state.wavePattern[0] = value;
    break;
  case WAVE_RAM_START + 1:
    state.wavePattern[1] = value;
    break;
  case WAVE_RAM_START + 2:
    state.wavePattern[2] = value;
    break;
  case WAVE_RAM_START + 3:
    state.wavePattern[3] = value;
    break;
  case WAVE_RAM_START + 4:
    state.wavePattern[4] = value;
    break;
  case WAVE_RAM_START + 5:
    state.wavePattern[5] = value;
    break;
  case WAVE_RAM_START + 6:
    state.wavePattern[6] = value;
    break;
  case WAVE_RAM_START + 7:
    state.wavePattern[7] = value;
    break;
  case WAVE_RAM_START + 8:
    state.wavePattern[8] = value;
    break;
  case WAVE_RAM_START + 9:
    state.wavePattern[9] = value;
    break;
  case WAVE_RAM_START + 10:
    state.wavePattern[10] = value;
    break;
  case WAVE_RAM_START + 11:
    state.wavePattern[11] = value;
    break;
  case WAVE_RAM_START + 12:
    state.wavePattern[12] = value;
    break;
  case WAVE_RAM_START + 13:
    state.wavePattern[13] = value;
    break;
  case WAVE_RAM_START + 14:
    state.wavePattern[14] = value;
    break;
  case WAVE_RAM_START + 15:
    state.wavePattern[15] = value;
    break;
  default:
    throw std::runtime_error("Invalid APU write");
    break;
  };
}

void APU::frameSequencerAction()
{
  frameTimer++;
  if (frameTimer == FRAME_SEQUENCER_CLOCK)
  {
    frameTimer = 0;
    frameSequence++;
    frameSequence %= FRAME_SEQUENCER_STEPS;

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

uint8_t APU::getChannel1Sample()
{
  if ((state.channel1.nrx4 & TRIGGER_BIT) != 0)
  {
    state.channel1.reset();
  }

  bool timerTriggered = state.channel1.timerAction();

  if (timerTriggered)
  {
    state.channel1.dutyAction();
  }

  state.channel1.updateTriggers(triggerLength, triggerEnvelope, triggerSweep);

  state.channel1.enabled &= state.channel1.lengthTimerAction();

  if (state.channel1.envelopeEnabled)
  {
    state.channel1.envelopeAction();
  }

  return state.channel1.getSample();
}

uint8_t APU::getChannel2Sample()
{
  if ((state.channel2.nrx4 & TRIGGER_BIT) != 0)
  {
    state.channel2.reset();
  }

  bool timerTriggered = state.channel2.timerAction();

  if (timerTriggered)
  {
    state.channel2.dutyAction();
  }

  state.channel2.updateTriggers(triggerLength, triggerEnvelope, triggerSweep);

  state.channel2.enabled &= state.channel2.lengthTimerAction();

  if (state.channel2.envelopeEnabled)
  {
    state.channel2.envelopeAction();
  }

  return state.channel2.getSample();
}

uint8_t APU::getChannel3Sample()
{
  if ((state.channel3.nrx4 & TRIGGER_BIT) != 0)
  {
    state.channel3.reset();
  }

  state.channel3.updateTriggers(triggerLength, triggerEnvelope, false);

  bool timerTriggered = state.channel3.timerAction();

  if (timerTriggered)
  {
    ++state.channel3.sample %= WAVE_SAMPLE_COUNT;
  }

  uint8_t sample = state.wavePattern[state.channel3.sample / 2];

  if (state.channel3.sample % 2)
  {
    sample = sample & WAVE_LOW_NIBBLE_MASK;
  }
  else
  {
    sample = sample >> NIBBLE_SIZE;
  }

  state.channel3.enabled &= state.channel3.lengthTimerAction();

  int shiftVol = ((state.channel3.nrx2 >> WAVE_VOLUME_SHIFT_POS) & 0x03) ? ((state.channel3.nrx2 >> WAVE_VOLUME_SHIFT_POS) & 0x03) - 1 : 4;

  sample >>= shiftVol;

  return state.channel3.getSample(sample);
}

uint8_t APU::getChannel4Sample()
{
  if ((state.channel4.nrx4 & TRIGGER_BIT) != 0)
  {
    state.channel4.reset();
  }

  state.channel4.updateTriggers(triggerLength, triggerEnvelope, false);

  state.channel4.timerAction();

  state.channel4.enabled &= state.channel4.lengthTimerAction();

  state.channel4.envelopeAction();

  return state.channel4.getSample();
}

uint8_t APU::getSample()
{
  uint8_t ch1Sample = 0;
  uint8_t ch2Sample = 0;
  uint8_t ch3Sample = 0;
  uint8_t ch4Sample = 0;

  for (int i = 0; i < SAMPLE_RATE; i++)
  {
    frameSequencerAction();
    ch1Sample = getChannel1Sample();
    ch2Sample = getChannel2Sample();
    ch3Sample = getChannel3Sample();
    ch4Sample = getChannel4Sample();
  }

  return ch1Sample + ch2Sample + ch3Sample + ch4Sample;
}