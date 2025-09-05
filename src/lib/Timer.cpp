#include "../../include/Timer.h"
#include "../../include/Cpu.h"

Timer::Timer(std::shared_ptr<CPU> cpu) : cpu(cpu)
{
  state.div = INITIAL_DIV_VALUE;
}

void Timer::tick()
{
  uint16_t prevDivider = state.div;

  state.div++;

  bool updateTimer = false;

  switch (state.tac & (FREQUENCY_BITS_MASK))
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

    if (state.tima == TIMA_OVERFLOW)
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
  case DIV_REGISTER:
    state.div = 0;
    break;

  case TIMA_REGISTER:
    state.tima = value;
    break;

  case TMA_REGISTER:
    state.tma = value;
    break;

  case TAC_REGISTER:
    state.tac = value;
    break;
  }
}

uint8_t Timer::read(uint16_t address)
{
  switch (address)
  {
  case DIV_REGISTER:
    return state.div >> 8;
  case TIMA_REGISTER:
    return state.tima;
  case TMA_REGISTER:
    return state.tma;
  case TAC_REGISTER:
    return state.tac;
  }
}