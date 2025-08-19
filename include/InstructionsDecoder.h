#pragma once

#include "./Instructions.h"

#include <memory>

class CPU;
class InstructionsDecoder
{
public:
  InstructionsDecoder(CPU *cpu);

  void decode(uint8_t opcode);
private:
  CPU *cpu;

  static const Instruction &getInstruction(uint8_t opcode);
};