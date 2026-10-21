// clang-format Language: Cpp
#pragma once

#include <cstdint>
#include <stdexcept>

#include "./Registers.h"
#include "Common.h"
#include "Interrupts.h"
#include "Serializable.h"

class IMemAccess;
class CPU : public Serializable {
 public:
  struct State {
    struct Registers registers;
    uint8_t opcode = 0;
    uint16_t opValue = 0;
    bool isMemoryOp = false;
    uint16_t memoryAddress = 0;
    bool ime = false;
    bool imeScheduled = false;
    bool halted = false;
    bool haltBug = false;
  };

  using CycleCallbackFn = void (*)(void*, int);

  CPU(Interrupts& interrupts, IMemAccess& memory, CycleCallbackFn cycleCallback,
      void* cycleCallbackCtx);

  void step();
  void save(std::ostream& out) const override;
  void load(std::istream& in) override;

 private:
  [[= serial::skip]] Interrupts& interrupts_;
  [[= serial::skip]] IMemAccess& memory_;
  [[= serial::skip]] CycleCallbackFn cycleCallback_;
  [[= serial::skip]] void* cycleCallbackCtx_;

  State state;

  void cycle(int n) { cycleCallback_(cycleCallbackCtx_, n); }

  uint8_t flagZ() const { return (state.registers.f >> 7) & 1; }
  uint8_t flagN() const { return (state.registers.f >> 6) & 1; }
  uint8_t flagH() const { return (state.registers.f >> 5) & 1; }
  uint8_t flagC() const { return (state.registers.f >> 4) & 1; }

  void setFlags(int z, int n, int h, int c) {
    if (z != -1) {
      if (z) {
        state.registers.f |= (1 << 7);
      } else {
        state.registers.f &= static_cast<uint8_t>(~(1 << 7));
      }
    }
    if (n != -1) {
      if (n) {
        state.registers.f |= (1 << 6);
      } else {
        state.registers.f &= static_cast<uint8_t>(~(1 << 6));
      }
    }
    if (h != -1) {
      if (h) {
        state.registers.f |= (1 << 5);
      } else {
        state.registers.f &= static_cast<uint8_t>(~(1 << 5));
      }
    }
    if (c != -1) {
      if (c) {
        state.registers.f |= (1 << 4);
      } else {
        state.registers.f &= static_cast<uint8_t>(~(1 << 4));
      }
    }
  }

  inline void stackPush8(uint8_t val);
  inline uint8_t stackPop8();
  inline uint16_t stackPop16();

  void handleInterrupts();
  bool checkInterrupt(uint16_t address, InterruptType type);
  void interruptHandle(uint16_t address);

  void executeCB();
};
