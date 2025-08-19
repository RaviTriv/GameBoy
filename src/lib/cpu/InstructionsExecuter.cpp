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

/*
<---ARITHMETIC-START--->
*/
void InstructionsExecuter::add()
{
  uint32_t value = cpu->readRegister16(cpu->state.instruction->reg1) + cpu->state.opValue;
  bool is16BIT = cpu->is16Bit(cpu->state.instruction->reg1);

  if (is16BIT)
  {
    cpu->cycleCallback(1);
  }

  if (cpu->state.instruction->reg1 == RegisterType::SP)
  {
    value = cpu->readRegister16(cpu->state.instruction->reg1) + (int8_t)cpu->state.opValue;
  }

  int z = (value & BYTE_MASK) == 0;
  int h = (cpu->readRegister16(cpu->state.instruction->reg1) & NIBBLE_MASK) + (cpu->state.opValue & NIBBLE_MASK) >= 0x10;
  int c = (int)(cpu->readRegister16(cpu->state.instruction->reg1) & BYTE_MASK) + (int)(cpu->state.opValue & BYTE_MASK) >= 0x100;

  if (is16BIT)
  {
    z = -1;
    h = (cpu->readRegister16(cpu->state.instruction->reg1) & 0xFFF) + (cpu->state.opValue & 0xFFF) >= 0x1000;
    uint32_t n = ((uint32_t)cpu->readRegister16(cpu->state.instruction->reg1)) + ((uint32_t)cpu->state.opValue);
    c = n >= 0x10000;
  }

  if (cpu->state.instruction->reg1 == RegisterType::SP)
  {
    z = 0;
    h = (cpu->readRegister16(cpu->state.instruction->reg1) & NIBBLE_MASK) + (cpu->state.opValue & NIBBLE_MASK) >= 0x10;
    c = (int)(cpu->readRegister16(cpu->state.instruction->reg1) & BYTE_MASK) + (int)(cpu->state.opValue & BYTE_MASK) >= 0x100;
  }

  cpu->setRegister16(cpu->state.instruction->reg1, value & 0xFFFF);
  cpu->setFlags(z, 0, h, c);
}

void InstructionsExecuter::adc()
{
  uint16_t val = cpu->state.opValue;
  uint16_t a = cpu->state.registers.a;
  uint16_t c = cpu->FLAG_C();

  cpu->state.registers.a = (a + val + c) & BYTE_MASK;

  cpu->setFlags(cpu->state.registers.a == 0, 0,
                (a & NIBBLE_MASK) + (val & NIBBLE_MASK) + c > NIBBLE_MASK,
                a + val + c > BYTE_MASK);
}

void InstructionsExecuter::dec()
{
  uint16_t value = cpu->readRegister16(cpu->state.instruction->reg1) - 1;

  if (cpu->is16Bit(cpu->state.instruction->reg1))
  {
    cpu->cycleCallback(1);
  }

  if (cpu->state.instruction->reg1 == RegisterType::HL && cpu->state.instruction->addressMode == AddressingMode::MR)
  {
    value = cpu->bus->read8(cpu->readRegister16(RegisterType::HL)) - 1;
    cpu->bus->write8(cpu->readRegister16(RegisterType::HL), value);
  }
  else
  {
    cpu->setRegister16(cpu->state.instruction->reg1, value);
    value = cpu->readRegister16(cpu->state.instruction->reg1);
  }

  if ((cpu->state.opcode & 0x0B) == 0x0B)
  {
    return;
  }

  cpu->setFlags(value == 0, 1, (value & 0x0F) == 0x0F, -1);
}

void InstructionsExecuter::inc()
{
  uint16_t value = cpu->readRegister16(cpu->state.instruction->reg1) + 1;

  if (cpu->is16Bit(cpu->state.instruction->reg1))
  {
    cpu->cycleCallback(1);
  }

  if (cpu->state.instruction->reg1 == RegisterType::HL && cpu->state.instruction->addressMode == AddressingMode::MR)
  {
    value = cpu->bus->read8(cpu->readRegister16(RegisterType::HL)) + 1;
    value &= BYTE_MASK;
    cpu->bus->write8(cpu->readRegister16(RegisterType::HL), value);
  }
  else
  {
    cpu->setRegister16(cpu->state.instruction->reg1, value);
    value = cpu->readRegister16(cpu->state.instruction->reg1);
  }

  if ((cpu->state.opcode & 0x03) == 0x03)
  {
    return;
  }

  cpu->setFlags(value == 0, 0, (value & 0x0F) == 0, -1);
}

void InstructionsExecuter::sub()
{
  uint16_t value = cpu->readRegister16(cpu->state.instruction->reg1) - cpu->state.opValue;

  int z = value == 0;
  int h = ((int)cpu->readRegister16(cpu->state.instruction->reg1) & NIBBLE_MASK) - ((int)cpu->state.opValue & NIBBLE_MASK) < 0;
  int c = ((int)cpu->readRegister16(cpu->state.instruction->reg1)) - ((int)cpu->state.opValue) < 0;

  cpu->setRegister16(cpu->state.instruction->reg1, value);
  cpu->setFlags(z, 1, h, c);
}

void InstructionsExecuter::sbc()
{
  uint8_t value = cpu->state.opValue + cpu->FLAG_C();

  int z = cpu->readRegister16(cpu->state.instruction->reg1) - value == 0;
  int h = ((int)cpu->readRegister16(cpu->state.instruction->reg1) & NIBBLE_MASK) - ((int)cpu->state.opValue & NIBBLE_MASK) - ((int)cpu->FLAG_C()) < 0;
  int c = ((int)cpu->readRegister16(cpu->state.instruction->reg1)) - ((int)cpu->state.opValue) - ((int)cpu->FLAG_C()) < 0;

  cpu->setRegister16(cpu->state.instruction->reg1, cpu->readRegister16(cpu->state.instruction->reg1) - value);
  cpu->setFlags(z, 1, h, c);
}

void InstructionsExecuter::cp()
{
  int tmp = (int)cpu->state.registers.a - (int)cpu->state.opValue;
  cpu->setFlags(tmp == 0, 1, ((int)cpu->state.registers.a & 0x0F) - ((int)cpu->state.opValue & 0x0F) < 0, tmp < 0);
}
/*
<---ARITHMETIC-END--->
*/