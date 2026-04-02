#pragma once

#include "./Instructions.h"
#include "./Registers.h"
#include "Common.h"

#include <cstdint>
#include <functional>

constexpr uint8_t CTX_FLAG_Z_BIT = 7;
constexpr uint8_t CTX_FLAG_N_BIT = 6;
constexpr uint8_t CTX_FLAG_H_BIT = 5;
constexpr uint8_t CTX_FLAG_C_BIT = 4;

class Bus;

struct CpuState {
  struct Registers &registers;
  Instruction &instruction;
  uint8_t &opcode;
  uint16_t &opValue;
  bool &isMemoryOp;
  uint16_t &memoryAddress;
  bool &ime;
  bool &imeScheduled;
  uint8_t &ie;
  uint8_t &intf;
  bool &halted;
};

class CpuContext {
public:
  using CycleCallback = std::function<void(int)>;

  CpuContext(CpuState state, CycleCallback &cycleCallback, Bus *&bus);

  CpuState state;
  CycleCallback &cycleCallback;
  Bus *&bus;

  uint8_t readRegister8(RegisterType reg) const;
  uint16_t readRegister16(RegisterType reg) const;
  void setRegister8(RegisterType reg, uint8_t value);
  void setRegister16(RegisterType reg, uint16_t value);

  void stackPush8(uint8_t value);
  void stackPush16(uint16_t value);
  uint8_t stackPop8();
  uint16_t stackPop16();

  void setFlags(int z, int n, int h, int c);
  bool is16Bit(RegisterType reg) const;
  int FLAG_Z() const;
  int FLAG_N() const;
  int FLAG_H() const;
  int FLAG_C() const;

  RegisterType decodeRegister(uint8_t value);
  bool conditionCheck() const;
  void jumpToAddress(uint16_t addr, bool pushPC);

private:
  void setBit(uint8_t value, uint8_t bit);
  bool isFlagSet(uint8_t flags, uint8_t bit) const;
};
