#pragma once

#include "./Instructions.h"

#include <functional>
#include <memory>

struct DecodedInstructionState
{
};

class CPU;
class InstructionsDecoder
{
public:
  void decode(uint8_t opcode);

  InstructionsDecoder(CPU *cpu);

private:
  CPU *cpu;

  static const Instruction &getInstruction(uint8_t opcode);
};