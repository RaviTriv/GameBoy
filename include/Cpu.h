#pragma once

#include "./CpuContext.h"
#include "./InstructionsDecoder.h"
#include "./InstructionsExecuter.h"
#include "./Interrupts.h"
#include "InterruptRegs.h"
#include "InterruptSink.h"

#include <cstdint>
#include <string>

class Bus;
class CPU : public InterruptSink {
public:
  struct State {
    struct Registers registers;
    Instruction instruction;
    uint8_t opcode;
    uint16_t opValue;
    bool isMemoryOp;
    uint16_t memoryAddress;
    bool ime;
    bool imeScheduled;
    uint8_t ie;
    uint8_t intf;
    bool halted;
  };

  using CycleCallbackFn = void(*)(void*, int);

  CPU(CycleCallbackFn cycleCallback, void *cycleCallbackCtx, Bus *bus);

  void step();
  void setBus(Bus *bus);
  void requestInterrupt(InterruptType type) override;
  uint8_t getInterruptEnable() const;
  void setInterruptEnable(uint8_t value);
  uint8_t getInterruptFlags() const;
  InterruptRegs getInterruptRegs();
  void setInterruptFlags(uint8_t value);
  CPU::State getState() const;
  void setState(const State &state);

private:
  CycleCallbackFn cycleCallback;
  void *cycleCallbackCtx;
  Bus *bus;

  State state;
  CpuContext context;
  InstructionsDecoder decoder;
  InstructionsExecuter executer;
  Interrupts interrupt;

  void fetch();
  void decode();
  void execute();
};
