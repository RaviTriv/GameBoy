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
    imp();
    return;
  }
  case AddressingMode::R:
  {
    r();
    return;
  }
  case AddressingMode::R_R:
  {
    r_r();
    return;
  }
  case AddressingMode::R_D8:
  {
    r_d8();
    return;
  }
  case AddressingMode::D16:
  case AddressingMode::R_D16:
  {
    r_d16();
    return;
  }
  case AddressingMode::MR_R:
  {
    mr_r();
    return;
  }
  case AddressingMode::R_MR:
  {
    r_mr();
    return;
  }
  case AddressingMode::R_HLI:
  {
    r_hli();
    return;
  }
  case AddressingMode::R_HLD:
  {
    r_hld();
    return;
  }
  case AddressingMode::HLI_R:
  {
    hli_r();
    return;
  }
  case AddressingMode::HLD_R:
  {
    hld_r();
    return;
  }
  case AddressingMode::R_A8:
  {
    r_a8();
    return;
  }
  case AddressingMode::A8_R:
  {
    a8_r();
    return;
  }
  case AddressingMode::D8:
  {
    d8();
    return;
  }
  case AddressingMode::HL_SPR:
  {
    hl_spr();
    return;
  }
  case AddressingMode::A16_R:
  case AddressingMode::D16_R:
  {
    a16_r();
    return;
  }
  case AddressingMode::MR_D8:
  {
    mr_d8();
    return;
  }
  case AddressingMode::MR:
  {
    mr();
    return;
  }
  case AddressingMode::R_A16:
  {
    r_a16();
    return;
  }
  case AddressingMode::NONE:
  {
    none();
    return;
  }
  default:
    throw std::runtime_error("Unknown CPU addressing mode");
  }
}

void InstructionsDecoder::none()
{
}

void InstructionsDecoder::imp()
{
}

void InstructionsDecoder::r()
{
  cpu->state.opValue = cpu->readRegister16(cpu->state.instruction.reg1);
}

void InstructionsDecoder::r_r()
{
  cpu->state.opValue = cpu->readRegister16(cpu->state.instruction.reg2);
}

void InstructionsDecoder::r_d8()
{
  cpu->state.opValue = cpu->bus->read8(cpu->state.registers.pc);
  cpu->cycleCallback(1);
  cpu->state.registers.pc++;
}
void InstructionsDecoder::r_d16()
{
  uint16_t low = cpu->bus->read8(cpu->state.registers.pc);
  cpu->cycleCallback(1);
  uint16_t high = cpu->bus->read8(cpu->state.registers.pc + 1);
  cpu->cycleCallback(1);
  cpu->state.opValue = low | (high << 8);
  cpu->state.registers.pc += 2;
}

void InstructionsDecoder::mr_r()
{
  cpu->state.memoryAddress = cpu->readRegister16(cpu->state.instruction.reg1);
  cpu->state.isMemoryOp = true;

  cpu->state.opValue = cpu->readRegister16(cpu->state.instruction.reg2);

  if (cpu->state.instruction.reg1 == RegisterType::C)
  {
    cpu->state.memoryAddress |= 0xFF00;
  }
}

void InstructionsDecoder::r_mr()
{
  uint16_t address = cpu->readRegister16(cpu->state.instruction.reg2);

  if (cpu->state.instruction.reg2 == RegisterType::C)
  {
    address |= 0xFF00;
  }

  cpu->state.opValue = cpu->bus->read8(address);
  cpu->cycleCallback(1);
}

void InstructionsDecoder::r_hli()
{
  cpu->state.opValue = cpu->bus->read8(cpu->readRegister16(cpu->state.instruction.reg2));
  cpu->cycleCallback(1);
  cpu->setRegister16(RegisterType::HL, cpu->readRegister16(RegisterType::HL) + 1);
}

void InstructionsDecoder::r_hld()
{
  cpu->state.opValue = cpu->bus->read8(cpu->readRegister16(cpu->state.instruction.reg2));
  cpu->cycleCallback(1);
  cpu->setRegister16(RegisterType::HL, cpu->readRegister16(RegisterType::HL) - 1);
}

void InstructionsDecoder::hli_r()
{
  cpu->state.memoryAddress = cpu->readRegister16(cpu->state.instruction.reg1);
  cpu->state.isMemoryOp = true;

  cpu->state.opValue = cpu->readRegister16(cpu->state.instruction.reg2);
  cpu->setRegister16(RegisterType::HL, cpu->readRegister16(RegisterType::HL) + 1);
}

void InstructionsDecoder::hld_r()
{
  cpu->state.memoryAddress = cpu->readRegister16(cpu->state.instruction.reg1);
  cpu->state.isMemoryOp = true;

  cpu->state.opValue = cpu->readRegister16(cpu->state.instruction.reg2);
  cpu->setRegister16(RegisterType::HL, cpu->readRegister16(RegisterType::HL) - 1);
}

void InstructionsDecoder::r_a8()
{
  cpu->state.opValue = cpu->bus->read8((cpu->state.registers.pc));
  cpu->cycleCallback(1);
  cpu->state.registers.pc++;
}

void InstructionsDecoder::a8_r()
{
  cpu->state.memoryAddress = cpu->bus->read8(cpu->state.registers.pc) | 0xFF00;
  cpu->state.isMemoryOp = true;
  cpu->cycleCallback(1);
  cpu->state.registers.pc++;
}

void InstructionsDecoder::d8()
{
  cpu->state.opValue = cpu->bus->read8(cpu->state.registers.pc);
  cpu->cycleCallback(1);
  cpu->state.registers.pc++;
}

void InstructionsDecoder::hl_spr()
{
  cpu->state.opValue = cpu->bus->read8(cpu->state.registers.pc);
  cpu->cycleCallback(1);
  cpu->state.registers.pc++;
}

void InstructionsDecoder::a16_r()
{
  uint16_t low = cpu->bus->read8(cpu->state.registers.pc);
  cpu->cycleCallback(1);
  uint16_t high = cpu->bus->read8(cpu->state.registers.pc + 1);
  cpu->cycleCallback(1);
  cpu->state.memoryAddress = low | (high << 8);
  cpu->state.isMemoryOp = true;
  cpu->state.registers.pc += 2;
  cpu->state.opValue = cpu->readRegister16(cpu->state.instruction.reg2);
}

void InstructionsDecoder::mr_d8()
{
  cpu->state.opValue = cpu->bus->read8(cpu->state.registers.pc);
  cpu->cycleCallback(1);
  cpu->state.registers.pc++;
  cpu->state.memoryAddress = cpu->readRegister16(cpu->state.instruction.reg1);
  cpu->state.isMemoryOp = true;
}

void InstructionsDecoder::mr()
{
  cpu->state.memoryAddress = cpu->readRegister16(cpu->state.instruction.reg1);
  cpu->state.isMemoryOp = true;
  cpu->state.opValue = cpu->bus->read8(cpu->state.memoryAddress);
  cpu->cycleCallback(1);
}

void InstructionsDecoder::r_a16()
{
  uint16_t low = cpu->bus->read8(cpu->state.registers.pc);
  cpu->cycleCallback(1);
  uint16_t high = cpu->bus->read8(cpu->state.registers.pc + 1);
  cpu->cycleCallback(1);
  cpu->state.opValue = cpu->bus->read8(low | (high << 8));
  cpu->cycleCallback(1);
  cpu->state.registers.pc += 2;
}