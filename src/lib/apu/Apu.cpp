#include "Apu.h"
#include "Channel.h"
#include "Logger.h"

uint8_t APU::read(uint16_t address) const {
  switch (address) {
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
  default:
    if (address >= WAVE_RAM_START && address <= WAVE_RAM_END) {
      return state.wavePattern[address - WAVE_RAM_START];
    }
    return 0xFF;
  }
}

void APU::write(uint16_t address, uint8_t value) {
  switch (address) {
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
    if (!state.enabled) {
      state.channel1 = {};
      state.channel2 = {};
      state.channel3 = {};
      state.channel4 = {};
    }
    break;
  default:
    if (address >= WAVE_RAM_START && address <= WAVE_RAM_END) {
      state.wavePattern[address - WAVE_RAM_START] = value;
      break;
    }
    break;
  };
}

uint8_t APU::getChannel3CurrentSample() {
  uint8_t sample = state.wavePattern[state.channel3.sample / 2];

  if (state.channel3.sample % 2) {
    sample = sample & WAVE_LOW_NIBBLE_MASK;
  } else {
    sample = sample >> NIBBLE_SIZE;
  }

  int shiftVol =
      ((state.channel3.nrx2 >> WAVE_VOLUME_SHIFT_POS) & 0x03)
          ? ((state.channel3.nrx2 >> WAVE_VOLUME_SHIFT_POS) & 0x03) - 1
          : 4;

  sample >>= shiftVol;

  return state.channel3.getSample(sample);
}

StereoSample APU::mixSample() {
  uint8_t ch1 = state.channel1.getSample();
  uint8_t ch2 = state.channel2.getSample();
  uint8_t ch3 = getChannel3CurrentSample();
  uint8_t ch4 = state.channel4.getSample();
  uint8_t nr51 = state.registers.NR51;
  uint8_t nr50 = state.registers.NR50;

  uint8_t left = 0;
  uint8_t right = 0;

  if (nr51 & NR51_CH1_LEFT) { left += ch1; }
  if (nr51 & NR51_CH2_LEFT) { left += ch2; }
  if (nr51 & NR51_CH3_LEFT) { left += ch3; }
  if (nr51 & NR51_CH4_LEFT) { left += ch4; }

  if (nr51 & NR51_CH1_RIGHT) { right += ch1; }
  if (nr51 & NR51_CH2_RIGHT) { right += ch2; }
  if (nr51 & NR51_CH3_RIGHT) { right += ch3; }
  if (nr51 & NR51_CH4_RIGHT) { right += ch4; }

  uint8_t leftVol = (nr50 >> NR50_LEFT_VOLUME_SHIFT) & NR50_VOLUME_MASK;
  uint8_t rightVol = nr50 & NR50_VOLUME_MASK;

  left = left * (leftVol + 1) / 8;
  right = right * (rightVol + 1) / 8;

  return {left, right};
}

void APU::flushChannelTimers() {
  if (pendingTicks == 0) {
    return;
  }

  int ticks = static_cast<int>(pendingTicks);
  pendingTicks = 0;

  if (state.channel1.enabled) {
    int fires = state.channel1.advanceTimer(ticks);
    if (fires > 0) {
      state.channel1.duty =
          (state.channel1.duty + fires) % SquareChannel::DUTY_CYCLE_STEPS;
    }
  }

  if (state.channel2.enabled) {
    int fires = state.channel2.advanceTimer(ticks);
    if (fires > 0) {
      state.channel2.duty =
          (state.channel2.duty + fires) % SquareChannel::DUTY_CYCLE_STEPS;
    }
  }

  if (state.channel3.enabled) {
    int fires = state.channel3.advanceTimer(ticks);
    if (fires > 0) {
      state.channel3.sample =
          (state.channel3.sample + fires) % WAVE_SAMPLE_COUNT;
    }
  }

  if (state.channel4.enabled) {
    state.channel4.advanceTimer(ticks);
  }
}

void APU::tick() {
  if ((state.channel1.nrx4 & TRIGGER_BIT) != 0) {
    flushChannelTimers();
    state.channel1.reset();
  }
  if ((state.channel2.nrx4 & TRIGGER_BIT) != 0) {
    flushChannelTimers();
    state.channel2.reset();
  }
  if ((state.channel3.nrx4 & TRIGGER_BIT) != 0) {
    flushChannelTimers();
    state.channel3.reset();
  }
  if ((state.channel4.nrx4 & TRIGGER_BIT) != 0) {
    flushChannelTimers();
    state.channel4.reset();
  }

  pendingTicks++;

  frameTimer++;
  if (frameTimer == FRAME_SEQUENCER_CLOCK) {
    frameTimer = 0;
    frameSequence++;
    frameSequence %= FRAME_SEQUENCER_STEPS;

    triggerLength = frameSequence % 2 == 0;
    triggerEnvelope = frameSequence == 7;
    triggerSweep = frameSequence == 2 || frameSequence == 6;

    flushChannelTimers();

    if (state.channel1.enabled) {
      state.channel1.updateTriggers(triggerLength, triggerEnvelope,
                                    triggerSweep);
      state.channel1.enabled &= state.channel1.lengthTimerAction();
      if (state.channel1.envelopeEnabled) {
        state.channel1.envelopeAction();
      }
      state.channel1.sweepAction();
    }

    if (state.channel2.enabled) {
      state.channel2.updateTriggers(triggerLength, triggerEnvelope,
                                    triggerSweep);
      state.channel2.enabled &= state.channel2.lengthTimerAction();
      if (state.channel2.envelopeEnabled) {
        state.channel2.envelopeAction();
      }
    }

    if (state.channel3.enabled) {
      state.channel3.updateTriggers(triggerLength, triggerEnvelope, false);
      state.channel3.enabled &= state.channel3.lengthTimerAction();
    }

    if (state.channel4.enabled) {
      state.channel4.updateTriggers(triggerLength, triggerEnvelope, false);
      state.channel4.enabled &= state.channel4.lengthTimerAction();
      state.channel4.envelopeAction();
    }
  }

  sampleTimer++;
  if (sampleTimer >= CPU_CYCLES_PER_SAMPLE) {
    sampleTimer = 0;
    flushChannelTimers();
    StereoSample sample = mixSample();
    sampleQueue.push(sample);
  }
}

std::size_t APU::popSamples(StereoSample *out, std::size_t count) {
  return sampleQueue.pop_n(out, count);
}