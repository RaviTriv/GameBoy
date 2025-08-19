#include "../../../include/InstructionsDecoder.h"

InstructionsDecoder::InstructionsDecoder(CycleCallback cycleCallback) : cycleCallback(cycleCallback) {};

const Instruction &InstructionsDecoder::getInstruction(uint8_t opcode)
{
  return INSTRUCTIONS[opcode];
}

void InstructionsDecoder::decode(uint8_t opcode)
{
  const Instruction &instruction = getInstruction(opcode);

  cycleCallback(1);

  // TODO: reset

  switch (instruction.addressMode)
  {
  case AddressingMode::IMP:
  {
    return;
  }
  case AddressingMode::R:
  {
  }
  case AddressingMode::R_R:
  {
  }
  case AddressingMode::R_D8:
  {
  }
  case AddressingMode::D16:
  case AddressingMode::R_D16:
  {
  }
  case AddressingMode::MR_R:
  {
  }
  case AddressingMode::R_MR:
  {
  }
  case AddressingMode::R_HLI:
  {
  }
  case AddressingMode::R_HLD:
  {
  }
  case AddressingMode::HLI_R:
  {
  }
  case AddressingMode::HLD_R:
  {
  }
  case AddressingMode::R_A8:
  {
  }
  case AddressingMode::A8_R:
  {
  }
  case AddressingMode::D8:
  {
  }
  case AddressingMode::HL_SPR:
  {
  }
  case AddressingMode::A16_R:
  case AddressingMode::D16_R:
  {
  }
  case AddressingMode::MR_D8:
  {
  }
  case AddressingMode::MR:
  {
  }
  case AddressingMode::R_A16:
  {
  }
  case AddressingMode::NONE:
  {
    return;
  }
  default:
    throw std::runtime_error("UNKNOWN ADDRESSING MODE");
  }
}