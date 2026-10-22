#include "Apu.h"

#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>

#include "Channel.h"
#include "Serializable.h"

uint8_t APU::readRegister(uint16_t address) const {
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
      return static_cast<uint8_t>((state.enabled ? ENABLE_BIT : 0x00) |
                                  (state.channel1.enabled ? 0x01 : 0x00) |
                                  (state.channel2.enabled ? 0x02 : 0x00) |
                                  (state.channel3.enabled ? 0x04 : 0x00) |
                                  (state.channel4.enabled ? 0x08 : 0x00));
    default:
      return 0x00;
  }
}

uint8_t APU::read(uint16_t address) const {
  if (address >= WAVE_RAM_START && address <= WAVE_RAM_END) {
    if (state.channel3.enabled) {
      return state.channel3.justFetched()
                 ? state.wavePattern[state.channel3.currentByte()]
                 : 0xFF;
    }
    return state.wavePattern[address - WAVE_RAM_START];
  }
  if (address < NR10_REGISTER || address >= WAVE_RAM_START) {
    return 0xFF;
  }
  return readRegister(address) | READ_MASKS[address - NR10_REGISTER];
}

bool APU::nextStepClocksLength() const { return (frameSequence & 1) != 0; }

bool APU::nextStepClocksEnvelope() const {
  return frameSequence == FRAME_STEP_BEFORE_ENVELOPE;
}

void APU::setPower(bool on) {
  if (on == state.enabled) {
    return;
  }
  flushChannelTimers();
  state.enabled = on;
  if (on) {
    frameSequence = FRAME_STEP_BEFORE_FIRST;
    state.channel1.duty = 0;
    state.channel2.duty = 0;
    state.channel3.sample = 0;
    return;
  }
  auto clear = [](Channel& channel) {
    channel.nrx0 = channel.nrx1 = channel.nrx2 = channel.nrx3 = channel.nrx4 =
        0;
    channel.enabled = false;
  };
  clear(state.channel1);
  clear(state.channel2);
  clear(state.channel3);
  clear(state.channel4);
  state.registers.NR50 = 0;
  state.registers.NR51 = 0;
}

void APU::writeWhileOff(uint16_t address, uint8_t value) {
  switch (address) {
    case NR11_REGISTER:
      state.channel1.lengthTimer =
          SQUARE_MAX_LENGTH - (value & SQUARE_LENGTH_MASK);
      break;
    case NR21_REGISTER:
      state.channel2.lengthTimer =
          SQUARE_MAX_LENGTH - (value & SQUARE_LENGTH_MASK);
      break;
    case NR31_REGISTER:
      state.channel3.lengthTimer = WAVE_MAX_LENGTH - value;
      break;
    case NR41_REGISTER:
      state.channel4.lengthTimer =
          SQUARE_MAX_LENGTH - (value & SQUARE_LENGTH_MASK);
      break;
    default:
      break;
  }
}

void APU::write(uint16_t address, uint8_t value) {
  if (address >= WAVE_RAM_START && address <= WAVE_RAM_END) {
    if (!state.channel3.enabled) {
      state.wavePattern[address - WAVE_RAM_START] = value;
    } else if (state.channel3.justFetched()) {
      state.wavePattern[state.channel3.currentByte()] = value;
    }
    return;
  }
  if (address == NR52_REGISTER) {
    setPower((value & ENABLE_BIT) != 0);
    return;
  }
  if (!state.enabled) {
    writeWhileOff(address, value);
    return;
  }

  flushChannelTimers();
  switch (address) {
    case NR10_REGISTER:
      state.channel1.writeSweep(value);
      break;
    case NR11_REGISTER:
      state.channel1.nrx1 = value;
      state.channel1.lengthTimer =
          SQUARE_MAX_LENGTH - (value & SQUARE_LENGTH_MASK);
      break;
    case NR12_REGISTER:
      state.channel1.nrx2 = value;
      state.channel1.enabled &= state.channel1.envelopeDacEnabled();
      break;
    case NR13_REGISTER:
      state.channel1.nrx3 = value;
      break;
    case NR14_REGISTER:
      if (state.channel1.writeControl(value, SQUARE_MAX_LENGTH,
                                      nextStepClocksLength())) {
        state.channel1.trigger(nextStepClocksEnvelope());
      }
      break;
    case NR21_REGISTER:
      state.channel2.nrx1 = value;
      state.channel2.lengthTimer =
          SQUARE_MAX_LENGTH - (value & SQUARE_LENGTH_MASK);
      break;
    case NR22_REGISTER:
      state.channel2.nrx2 = value;
      state.channel2.enabled &= state.channel2.envelopeDacEnabled();
      break;
    case NR23_REGISTER:
      state.channel2.nrx3 = value;
      break;
    case NR24_REGISTER:
      if (state.channel2.writeControl(value, SQUARE_MAX_LENGTH,
                                      nextStepClocksLength())) {
        state.channel2.trigger(nextStepClocksEnvelope());
      }
      break;
    case NR30_REGISTER:
      state.channel3.nrx0 = value;
      if ((value & ENABLE_BIT) == 0) {
        state.channel3.enabled = false;
      }
      break;
    case NR31_REGISTER:
      state.channel3.nrx1 = value;
      state.channel3.lengthTimer = WAVE_MAX_LENGTH - value;
      break;
    case NR32_REGISTER:
      state.channel3.nrx2 = value;
      break;
    case NR33_REGISTER:
      state.channel3.nrx3 = value;
      break;
    case NR34_REGISTER:
      if (state.channel3.writeControl(value, WAVE_MAX_LENGTH,
                                      nextStepClocksLength())) {
        state.channel3.trigger(state.wavePattern);
      }
      break;
    case NR41_REGISTER:
      state.channel4.nrx1 = value;
      state.channel4.lengthTimer =
          SQUARE_MAX_LENGTH - (value & SQUARE_LENGTH_MASK);
      break;
    case NR42_REGISTER:
      state.channel4.nrx2 = value;
      state.channel4.enabled &= state.channel4.envelopeDacEnabled();
      break;
    case NR43_REGISTER:
      state.channel4.nrx3 = value;
      break;
    case NR44_REGISTER:
      if (state.channel4.writeControl(value, SQUARE_MAX_LENGTH,
                                      nextStepClocksLength())) {
        state.channel4.trigger(nextStepClocksEnvelope());
      }
      break;
    case NR50_REGISTER:
      state.registers.NR50 = value;
      break;
    case NR51_REGISTER:
      state.registers.NR51 = value;
      break;
    default:
      break;
  }
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

  sample = static_cast<uint8_t>(sample >> shiftVol);

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

  if (nr51 & NR51_CH1_LEFT) {
    left += ch1;
  }
  if (nr51 & NR51_CH2_LEFT) {
    left += ch2;
  }
  if (nr51 & NR51_CH3_LEFT) {
    left += ch3;
  }
  if (nr51 & NR51_CH4_LEFT) {
    left += ch4;
  }

  if (nr51 & NR51_CH1_RIGHT) {
    right += ch1;
  }
  if (nr51 & NR51_CH2_RIGHT) {
    right += ch2;
  }
  if (nr51 & NR51_CH3_RIGHT) {
    right += ch3;
  }
  if (nr51 & NR51_CH4_RIGHT) {
    right += ch4;
  }

  uint8_t leftVol = (nr50 >> NR50_LEFT_VOLUME_SHIFT) & NR50_VOLUME_MASK;
  uint8_t rightVol = nr50 & NR50_VOLUME_MASK;

  left = static_cast<uint8_t>(left * (leftVol + 1) / 8);
  right = static_cast<uint8_t>(right * (rightVol + 1) / 8);

  return {.left = left, .right = right};
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
      state.channel1.duty = static_cast<uint8_t>(
          (state.channel1.duty + fires) % SquareChannel::DUTY_CYCLE_STEPS);
    }
  }

  if (state.channel2.enabled) {
    int fires = state.channel2.advanceTimer(ticks);
    if (fires > 0) {
      state.channel2.duty = static_cast<uint8_t>(
          (state.channel2.duty + fires) % SquareChannel::DUTY_CYCLE_STEPS);
    }
  }

  if (state.channel4.enabled) {
    state.channel4.advanceTimer(ticks);
  }
}

void APU::tick() {
  pendingTicks++;
  if (state.channel3.enabled) {
    state.channel3.advanceTimer(1);
  }

  frameTimer++;
  if (frameTimer == FRAME_SEQUENCER_CLOCK) {
    frameTimer = 0;
    if (state.enabled) {
      frameSequence++;
      frameSequence %= FRAME_SEQUENCER_STEPS;

      flushChannelTimers();

      if (frameSequence % 2 == 0) {
        state.channel1.clockLength();
        state.channel2.clockLength();
        state.channel3.clockLength();
        state.channel4.clockLength();
      }
      if ((frameSequence == 2 || frameSequence == 6) &&
          state.channel1.enabled) {
        state.channel1.sweepAction();
      }
      if (frameSequence == 7) {
        state.channel1.envelopeAction();
        state.channel2.envelopeAction();
        state.channel4.envelopeAction();
      }
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

std::size_t APU::popSamples(StereoSample* out, std::size_t count) {
  return sampleQueue.pop_n(out, count);
}
void APU::save(std::ostream& out) const { serial::saveFields(out, *this); }

void APU::load(std::istream& in) { serial::loadFields(in, *this); }
