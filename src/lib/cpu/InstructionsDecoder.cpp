#include "../../../include/InstructionsDecoder.h"

const Instruction &InstructionsDecoder::getInstruction(uint8_t opcode)
{
  return INSTRUCTIONS[opcode];
}