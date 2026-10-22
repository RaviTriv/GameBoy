#include "Timer.h"

#include <cstdint>
#include <istream>
#include <ostream>

#include "Common.h"
#include "Interrupts.h"
#include "Serializable.h"

Timer::Timer(Interrupts& interrupts) : interrupts_(interrupts) {
  state.div = INITIAL_DIV_VALUE;
}

bool Timer::timerSignal() const {
  return (state.tac & TIMER_ENABLE_BIT) &&
         (state.div & FREQUENCY_BITS[state.tac & FREQUENCY_BITS_MASK]);
}

void Timer::incrementTima() {
  state.tima++;
  if (state.tima == 0) {
    state.reload = Reload::PENDING;
  }
}

void Timer::advanceReload() {
  if (state.reload == Reload::PENDING) {
    state.reload = Reload::DONE;
    state.tima = state.tma;
    interrupts_.requestLate(InterruptType::TIMER);
  } else if (state.reload == Reload::DONE) {
    state.reload = Reload::NONE;
  }
}

void Timer::tick() {
  if ((state.div & M_CYCLE_MASK) == 0) {
    advanceReload();
  }

  bool before = timerSignal();
  state.div++;
  if (before && !timerSignal()) {
    incrementTima();
  }
}

void Timer::write(uint16_t address, uint8_t value) {
  bool before = timerSignal();
  switch (address) {
    case DIV_REGISTER:
      state.div = 0;
      break;

    case TIMA_REGISTER:
      if (state.reload != Reload::DONE) {
        state.tima = value;
        state.reload = Reload::NONE;
      }
      break;

    case TMA_REGISTER:
      state.tma = value;
      if (state.reload == Reload::DONE) {
        state.tima = value;
      }
      break;

    case TAC_REGISTER:
      state.tac = value;
      break;
    default:
      break;
  }
  if (before && !timerSignal()) {
    incrementTima();
  }
}

uint8_t Timer::read(uint16_t address) const {
  switch (address) {
    case DIV_REGISTER:
      return highByte(state.div);
    case TIMA_REGISTER:
      return state.tima;
    case TMA_REGISTER:
      return state.tma;
    case TAC_REGISTER:
      return state.tac | TAC_UNUSED_BITS;
    default:
      return 0;
  }
}
void Timer::save(std::ostream& out) const { serial::saveFields(out, *this); }

void Timer::load(std::istream& in) { serial::loadFields(in, *this); }
