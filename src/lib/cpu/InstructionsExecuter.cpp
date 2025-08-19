#include "../../../include/InstructionsExecuter.h"
#include "../../../include/Cpu.h"
#include "../../../include/Logger.h"

InstructionsExecuter::InstructionsExecuter(CPU *cpu) : cpu(cpu) {}

void InstructionsExecuter::execute()
{
  switch (cpu->state.instruction.type)
  {
  case InstructionType::NOP:
    cpu->cycleCallback(1);
    break;
  case InstructionType::XOR:
    break;
  case InstructionType::JP:
    break;
  case InstructionType::DI:
    break;
  case InstructionType::JR:
    break;
  case InstructionType::CALL:
    break;
  case InstructionType::LD:
    break;
  case InstructionType::LDH:
    break;
  case InstructionType::POP:
    break;
  case InstructionType::PUSH:
    break;
  case InstructionType::RET:
    break;
  case InstructionType::RETI:
    break;
  case InstructionType::RST:
    break;
  case InstructionType::INC:
    break;
  case InstructionType::DEC:
    break;
  case InstructionType::ADD:
    break;
  case InstructionType::ADC:
    break;
  case InstructionType::SUB:
    break;
  case InstructionType::SBC:
    break;
  case InstructionType::AND:
    break;
  case InstructionType::OR:
    break;
  case InstructionType::CP:
    break;
  case InstructionType::CB:
    break;
  case InstructionType::RLCA:
    break;
  case InstructionType::RRCA:
    break;
  case InstructionType::RLA:
    break;
  case InstructionType::RRA:
    break;
  case InstructionType::DAA:
    break;
  case InstructionType::CPL:
    break;
  case InstructionType::SCF:
    break;
  case InstructionType::CCF:
    break;
  case InstructionType::EI:
    break;
  case InstructionType::HALT:
    break;
  case InstructionType::NONE:
    throw std::runtime_error("Unknown Instruction Type");
    break;
  default:
    throw std::runtime_error("Unknown Instruction Type");
  }
}