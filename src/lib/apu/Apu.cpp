#include "../../../include/Apu.h"
#include "../../../include/Bus.h"
#include "../../../include/Channel.h"
#include "../../../include/Logger.h"

APU::APU(std::shared_ptr<Bus> bus) : bus(bus)
{
  state.channel1 = SquareChannel();
}

void APU::setBus(std::shared_ptr<Bus> bus) { this->bus = bus; };

void APU::write(uint16_t address, uint8_t value)
{
  // TODO: Implement APU write
  Logger::GetLogger()->info("APU WRITE");
}

uint8_t APU::read(uint16_t address)
{
  // TODO: Implement APU read
  Logger::GetLogger()->info("APU READ");
  return 0;
}

uint8_t APU::getChannel1Sample()
{
  // Can i read state directly or use io?, use constant?
  state.channel1.nrx1 = bus->read8(0xFF11);
  state.channel1.nrx2 = bus->read8(0xFF12);
  state.channel1.nrx3 = bus->read8(0xFF13);
  state.channel1.nrx4 = bus->read8(0xFF14);

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

  return state.channel1.getSample();
}

uint8_t APU::getSample()
{
  uint8_t ch1Sample = 0;

  for (int i = 0; i < SAMPLE_RATE; i++)
  {
    ch1Sample = getChannel1Sample();
  }
  return ch1Sample;
}