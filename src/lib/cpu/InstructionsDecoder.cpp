#include "../../../include/InstructionsDecoder.h"
#include "../../../include/Bus.h"
#include "../../../include/Cpu.h"

InstructionsDecoder::InstructionsDecoder(CPU *cpu) : cpu(cpu) {}

const Instruction &InstructionsDecoder::getInstruction(uint8_t opcode)
{
  return INSTRUCTIONS[opcode];
}

void InstructionsDecoder::decode(uint8_t opcode)
{
  const Instruction &instruction = getInstruction(opcode);
  cpu->state.instruction = instruction;

  cpu->cycleCallback(1);

  cpu->state.memoryAddress = 0;
  cpu->state.isMemoryOp = false;

  switch (instruction.addressMode)
  {
  case AddressingMode::IMP:
  {
    return;
  }
  case AddressingMode::R:
  {
    cpu->state.opValue = cpu->readRegister16(instruction.reg1);
    return;
  }
  case AddressingMode::R_R:
  {
    cpu->state.opValue = cpu->readRegister16(instruction.reg2);
    return;
  }
  case AddressingMode::R_D8:
  {
    cpu->state.opValue = cpu->bus->read8(cpu->state.registers.pc);
    cpu->cycleCallback(1);
    cpu->state.registers.pc++;
    return;
  }
  case AddressingMode::D16:
  case AddressingMode::R_D16:
  {
    uint16_t low = cpu->bus->read8(cpu->state.registers.pc);
    cpu->cycleCallback(1);
    uint16_t high = cpu->bus->read8(cpu->state.registers.pc + 1);
    cpu->cycleCallback(1);
    cpu->state.opValue = low | (high << 8);
    cpu->state.registers.pc += 2;
    return;
  }
  case AddressingMode::MR_R:
  {
    cpu->state.memoryAddress = cpu->readRegister16(instruction.reg1);
    cpu->state.isMemoryOp = true;

    cpu->state.opValue = cpu->readRegister16(instruction.reg2);

    if (instruction.reg1 == RegisterType::C)
    {
      cpu->state.memoryAddress |= 0xFF00;
    }
    return;
  }
  case AddressingMode::R_MR:
  {
    uint16_t address = cpu->readRegister16(instruction.reg2);

    if (instruction.reg2 == RegisterType::C)
    {
      address |= 0xFF00;
    }

    cpu->state.opValue = cpu->bus->read8(address);

    cpu->cycleCallback(1);

    return;
  }
  case AddressingMode::R_HLI:
  {
    cpu->state.opValue = cpu->bus->read8(cpu->readRegister16(instruction.reg2));

    cpu->cycleCallback(1);

    cpu->setRegister16(RegisterType::HL, cpu->readRegister16(RegisterType::HL) + 1);
    return;
  }
  case AddressingMode::R_HLD:
  {
    cpu->state.opValue = cpu->bus->read8(cpu->readRegister16(instruction.reg2));
    cpu->cycleCallback(1);
    cpu->setRegister16(RegisterType::HL, cpu->readRegister16(RegisterType::HL) - 1);
    return;
  }
  case AddressingMode::HLI_R:
  {
    cpu->state.memoryAddress = cpu->readRegister16(instruction.reg1);
    cpu->state.isMemoryOp = true;

    cpu->state.opValue = cpu->readRegister16(instruction.reg2);
    cpu->setRegister16(RegisterType::HL, cpu->readRegister16(RegisterType::HL) + 1);
    return;
  }
  case AddressingMode::HLD_R:
  {
    cpu->state.memoryAddress = cpu->readRegister16(instruction.reg1);
    cpu->state.isMemoryOp = true;

    cpu->state.opValue = cpu->readRegister16(instruction.reg2);
    cpu->setRegister16(RegisterType::HL, cpu->readRegister16(RegisterType::HL) - 1);
    return;
  }
  case AddressingMode::R_A8:
  {
    cpu->state.opValue = cpu->bus->read8(cpu->state.registers.pc);
    cpu->cycleCallback(1);
    cpu->state.registers.pc++;
    return;
  }
  case AddressingMode::A8_R:
  {
    cpu->state.memoryAddress = cpu->bus->read8(cpu->state.registers.pc) | 0xFF00;
    cpu->state.isMemoryOp = true;
    cpu->cycleCallback(1);
    cpu->state.registers.pc++;
    return;
  }
  case AddressingMode::D8:
  {
    cpu->state.opValue = cpu->bus->read8(cpu->state.registers.pc);
    cpu->cycleCallback(1);
    cpu->state.registers.pc++;
    return;
  }
  case AddressingMode::HL_SPR:
  {
    cpu->state.opValue = cpu->bus->read8(cpu->state.registers.pc);
    cpu->cycleCallback(1);
    cpu->state.registers.pc++;
    return;
  }
  case AddressingMode::A16_R:
  case AddressingMode::D16_R:
  {
    uint16_t low = cpu->bus->read8(cpu->state.registers.pc);
    cpu->cycleCallback(1);
    uint16_t high = cpu->bus->read8(cpu->state.registers.pc + 1);
    cpu->cycleCallback(1);
    cpu->state.memoryAddress = low | (high << 8);
    cpu->state.isMemoryOp = true;
    cpu->state.registers.pc += 2;
    cpu->state.opValue = cpu->readRegister16(instruction.reg2);
    return;
  }
  case AddressingMode::MR_D8:
  {
    cpu->state.opValue = cpu->bus->read8(cpu->state.registers.pc);
    cpu->cycleCallback(1);
    cpu->state.registers.pc++;
    cpu->state.memoryAddress = cpu->readRegister16(instruction.reg1);
    cpu->state.isMemoryOp = true;
    return;
  }
  case AddressingMode::MR:
  {
    cpu->state.memoryAddress = cpu->readRegister16(instruction.reg1);
    cpu->state.isMemoryOp = true;
    cpu->state.opValue = cpu->bus->read8(cpu->state.memoryAddress);
    cpu->cycleCallback(1);
    return;
  }
  case AddressingMode::R_A16:
  {
    uint16_t low = cpu->bus->read8(cpu->state.registers.pc);
    cpu->cycleCallback(1);
    uint16_t high = cpu->bus->read8(cpu->state.registers.pc + 1);
    cpu->cycleCallback(1);
    cpu->state.memoryAddress = low | (high << 8);
    cpu->state.registers.pc += 2;
    cpu->state.opValue = cpu->bus->read8(cpu->state.memoryAddress);
    cpu->cycleCallback(1);
    return;
  }
  case AddressingMode::NONE:
  {
    return;
  }
  default:
    throw std::runtime_error("Unknown CPU addressing mode");
  }
}