#pragma once

#include "./InstructionsDecoder.h"
#include "./InstructionsExecuter.h"

#include <cstdint>
#include <memory>
#include <functional>

constexpr uint8_t FLAG_Z_BIT = 7;
constexpr uint8_t FLAG_N_BIT = 6;
constexpr uint8_t FLAG_H_BIT = 5;
constexpr uint8_t FLAG_C_BIT = 4;
constexpr uint8_t NIBBLE_MASK = 0xF;

struct Registers
{
  uint8_t a;
  uint8_t f;
  uint8_t b;
  uint8_t c;
  uint8_t d;
  uint8_t e;
  uint8_t h;
  uint8_t l;
  uint16_t pc;
  uint16_t sp;
};

class Bus;
class CPU
{
public:
  struct State
  {
    struct Registers registers;
    Instruction instruction;
    uint8_t opcode;
    uint16_t opValue;
    bool isMemoryOp;
    uint16_t memoryAddress;
  };

  using CycleCallback = std::function<void(int)>;

  CPU(CycleCallback cycleCallback, std::shared_ptr<Bus> bus);

  void step();

private:
  CycleCallback cycleCallback;
  friend class InstructionsDecoder;
  InstructionsDecoder decoder;
  friend class InstructionsExecuter;
  InstructionsExecuter executer;
  std::shared_ptr<Bus> bus;

  State state;
  void fetch();
  void decode();
  void execute();

  uint8_t readRegister8(RegisterType reg) const;
  uint16_t readRegister16(RegisterType reg) const;
  void setRegister8(RegisterType reg, uint8_t value);
  void setRegister16(RegisterType reg, uint16_t value);
  void setBit(uint8_t value, uint8_t bit);
  void setFlags(int z, int n, int h, int c);
};