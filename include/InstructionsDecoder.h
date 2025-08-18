#include "./Instructions.h"

class InstructionsDecoder
{
public:
  static const Instruction &getInstruction(uint8_t opcode);
};