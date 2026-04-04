#pragma once

#include "./Instructions.h"
#include "./Registers.h"
#include "Common.h"
#include "InterruptRegs.h"
#include "InterruptSink.h"

#include <cstdint>
#include <stdexcept>

class Bus;
class CPU : public InterruptSink {
public:
  struct State {
    struct Registers registers;
    Instruction instruction{};
    uint8_t opcode = 0;
    uint16_t opValue = 0;
    bool isMemoryOp = false;
    uint16_t memoryAddress = 0;
    bool ime = false;
    bool imeScheduled = false;
    uint8_t ie = 0;
    uint8_t intf = 0;
    bool halted = false;
  };

  using CycleCallbackFn = void(*)(void*, int);

  CPU(CycleCallbackFn cycleCallback, void *cycleCallbackCtx, Bus *bus);

  void step();
  void setBus(Bus *bus);
  void requestInterrupt(InterruptType type) override;
  [[nodiscard]] uint8_t getInterruptEnable() const;
  void setInterruptEnable(uint8_t value);
  [[nodiscard]] uint8_t getInterruptFlags() const;
  [[nodiscard]] InterruptRegs getInterruptRegs();
  void setInterruptFlags(uint8_t value);
  [[nodiscard]] CPU::State getState() const;
  void setState(const State &state);

private:
  CycleCallbackFn cycleCallback;
  void *cycleCallbackCtx;
  Bus *bus;

  State state;

  inline void cycle(int n) { cycleCallback(cycleCallbackCtx, n); }

  inline int flagZ() const { return (state.registers.f >> 7) & 1; }
  inline int flagN() const { return (state.registers.f >> 6) & 1; }
  inline int flagH() const { return (state.registers.f >> 5) & 1; }
  inline int flagC() const { return (state.registers.f >> 4) & 1; }

  inline void setFlags(int z, int n, int h, int c) {
    if (z != -1) { if (z) state.registers.f |= (1 << 7); else state.registers.f &= ~(1 << 7); }
    if (n != -1) { if (n) state.registers.f |= (1 << 6); else state.registers.f &= ~(1 << 6); }
    if (h != -1) { if (h) state.registers.f |= (1 << 5); else state.registers.f &= ~(1 << 5); }
    if (c != -1) { if (c) state.registers.f |= (1 << 4); else state.registers.f &= ~(1 << 4); }
  }

  inline void stackPush8(uint8_t val);
  inline void stackPush16(uint16_t val);
  inline uint8_t stackPop8();
  inline uint16_t stackPop16();

  void handleInterrupts();
  bool checkInterrupt(uint16_t address, InterruptType type);
  void interruptHandle(uint16_t address);

  void executeCB();
};
