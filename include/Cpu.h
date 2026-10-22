// clang-format Language: Cpp
#pragma once

#include <cstdint>
#include <functional>
#include <stdexcept>

#include "./Registers.h"
#include "Common.h"
#include "Interrupts.h"
#include "Serializable.h"

class IMemAccess;
class CPU {
 public:
  struct State {
    struct Registers registers;
    bool ime = false;
    bool imeScheduled = false;
    bool halted = false;
    bool haltBug = false;
  };

  CPU(Interrupts& interrupts, IMemAccess& memory,
      std::function_ref<void(int)> cycleCallback);

  void step();
  [[nodiscard]] const Registers& registers() const { return state.registers; }
  void save(std::ostream& out) const;
  void load(std::istream& in);

 private:
  [[= serial::skip]] Interrupts& interrupts_;
  [[= serial::skip]] IMemAccess& memory_;
  [[= serial::skip]] std::function_ref<void(int)> cycleCallback_;

  State state;

  void cycle(int n) { cycleCallback_(n); }

  uint8_t flagZ() const { return (state.registers.f >> 7) & 1; }
  uint8_t flagN() const { return (state.registers.f >> 6) & 1; }
  uint8_t flagH() const { return (state.registers.f >> 5) & 1; }
  uint8_t flagC() const { return (state.registers.f >> 4) & 1; }

  void setFlag(int value, uint8_t mask) {
    if (value == -1) {
      return;
    }
    if (value) {
      state.registers.f |= mask;
    } else {
      state.registers.f &= static_cast<uint8_t>(~mask);
    }
  }

  void setFlags(int z, int n, int h, int c) {
    setFlag(z, 1 << 7);
    setFlag(n, 1 << 6);
    setFlag(h, 1 << 5);
    setFlag(c, 1 << 4);
  }

  inline void stackPush8(uint8_t val);
  inline uint8_t stackPop8();

  inline uint8_t fetch8();
  inline uint16_t fetch16();
  inline void push16(uint16_t value);
  inline uint16_t pop16();
  inline void call(uint16_t address);
  inline void ret();
  inline void halt();

  template <uint8_t Idx>
  inline uint8_t readR8();
  template <uint8_t Idx>
  inline void writeR8(uint8_t value);
  template <uint8_t P>
  inline uint16_t rp() const;
  template <uint8_t P>
  inline void setRp(uint16_t value);
  template <uint8_t P>
  inline uint16_t rp2() const;
  template <uint8_t P>
  inline void setRp2(uint16_t value);
  template <uint8_t Cc>
  inline bool cond() const;
  template <uint8_t Op>
  inline void alu(uint8_t value);
  template <uint8_t Op>
  inline uint8_t shift(uint8_t value);
  template <uint8_t Op>
  inline void execute();
  template <uint8_t Op>
  inline void executeCB();

  void handleInterrupts();
  bool checkInterrupt(uint16_t address, InterruptType type);
  void interruptHandle(uint16_t address);

  void dispatchCB();
};
