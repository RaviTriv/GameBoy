#include "../../include/Timer.h"
#include "../../include/Cpu.h"

Timer::Timer(std::shared_ptr<CPU> cpu) : cpu(cpu)
{
  state.div = 0xAC00;
}

void Timer::setDiv(uint16_t value)
{
  state.div = value;
}

void Timer::tick()
{
  uint16_t prevDivider = state.div;

  state.div++;

  bool updateTimer = false;

  switch (state.tac & (0b11))
  {
  case 0b00:
    updateTimer = (prevDivider & (1 << 9)) && (!(state.div & (1 << 9)));
    break;
  case 0b01:
    updateTimer = (prevDivider & (1 << 3)) && (!(state.div & (1 << 3)));
    break;
  case 0b10:
    updateTimer = (prevDivider & (1 << 5)) && (!(state.div & (1 << 5)));
    break;
  case 0b11:
    updateTimer = (prevDivider & (1 << 7)) && (!(state.div & (1 << 7)));
    break;
  }

  if (updateTimer && state.tac & (1 << 2))
  {
    state.tima++;

    if (state.tima == 0xFF)
    {
      state.tima = state.tma;

      cpu->requestInterrupt(InterruptType::TIMER);
    }
  }
}

void Timer::write(uint16_t address, uint8_t value)
{
  switch (address)
  {
  case 0xFF04:
    state.div = 0;
    break;

  case 0xFF05:
    state.tima = value;
    break;

  case 0xFF06:
    state.tma = value;
    break;

  case 0xFF07:
    state.tac = value;
    break;
  }
}

uint8_t Timer::read(uint16_t address)
{
  switch (address)
  {
  case 0xFF04:
    return state.div >> 8;
  case 0xFF05:
    return state.tima;
  case 0xFF06:
    return state.tma;
  case 0xFF07:
    return state.tac;
  }
}