#include "../../../include/Apu.h"
#include "../../../include/Channel.h"
#include "../../../include/Logger.h"

APU::APU()
{
  state.channel1 = SquareChannel();
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

uint8_t APU::getSample()
{
  uint8_t ch1Sample = 0;
  uint8_t ch2Sample = 0;

  for (int i = 0; i < SAMPLE_RATE; i++)
  {
    ch1Sample = getChannel1Sample();
    ch2Sample = getChannel2Sample();
  }

  return ch1Sample + ch2Sample;
}