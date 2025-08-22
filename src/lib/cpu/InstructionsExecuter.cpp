#include "../../../include/InstructionsExecuter.h"
#include "../../../include/Bus.h"
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
  uint32_t value = cpu->readRegister16(cpu->state.instruction.reg1) + cpu->state.opValue;
  bool is16BIT = cpu->is16Bit(cpu->state.instruction.reg1);

  if (is16BIT)
  {
    cpu->cycleCallback(1);
  }

  if (cpu->state.instruction.reg1 == RegisterType::SP)
  {
    value = cpu->readRegister16(cpu->state.instruction.reg1) + (int8_t)cpu->state.opValue;
  }

  int z = (value & BYTE_MASK) == 0;
  int h = (cpu->readRegister16(cpu->state.instruction.reg1) & NIBBLE_MASK) + (cpu->state.opValue & NIBBLE_MASK) >= 0x10;
  int c = (int)(cpu->readRegister16(cpu->state.instruction.reg1) & BYTE_MASK) + (int)(cpu->state.opValue & BYTE_MASK) >= 0x100;

  if (is16BIT)
  {
    z = -1;
    h = (cpu->readRegister16(cpu->state.instruction.reg1) & 0xFFF) + (cpu->state.opValue & 0xFFF) >= 0x1000;
    uint32_t n = ((uint32_t)cpu->readRegister16(cpu->state.instruction.reg1)) + ((uint32_t)cpu->state.opValue);
    c = n >= 0x10000;
  }

  if (cpu->state.instruction.reg1 == RegisterType::SP)
  {
    z = 0;
    h = (cpu->readRegister16(cpu->state.instruction.reg1) & NIBBLE_MASK) + (cpu->state.opValue & NIBBLE_MASK) >= 0x10;
    c = (int)(cpu->readRegister16(cpu->state.instruction.reg1) & BYTE_MASK) + (int)(cpu->state.opValue & BYTE_MASK) >= 0x100;
  }

  cpu->setRegister16(cpu->state.instruction.reg1, value & 0xFFFF);
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
  uint16_t value = cpu->readRegister16(cpu->state.instruction.reg1) - 1;

  if (cpu->is16Bit(cpu->state.instruction.reg1))
  {
    cpu->cycleCallback(1);
  }

  if (cpu->state.instruction.reg1 == RegisterType::HL && cpu->state.instruction.addressMode == AddressingMode::MR)
  {
    value = cpu->bus->read8(cpu->readRegister16(RegisterType::HL)) - 1;
    cpu->bus->write8(cpu->readRegister16(RegisterType::HL), value);
  }
  else
  {
    cpu->setRegister16(cpu->state.instruction.reg1, value);
    value = cpu->readRegister16(cpu->state.instruction.reg1);
  }

  if ((cpu->state.opcode & 0x0B) == 0x0B)
  {
    return;
  }

  cpu->setFlags(value == 0, 1, (value & 0x0F) == 0x0F, -1);
}

void InstructionsExecuter::inc()
{
  uint16_t value = cpu->readRegister16(cpu->state.instruction.reg1) + 1;

  if (cpu->is16Bit(cpu->state.instruction.reg1))
  {
    cpu->cycleCallback(1);
  }

  if (cpu->state.instruction.reg1 == RegisterType::HL && cpu->state.instruction.addressMode == AddressingMode::MR)
  {
    value = cpu->bus->read8(cpu->readRegister16(RegisterType::HL)) + 1;
    value &= BYTE_MASK;
    cpu->bus->write8(cpu->readRegister16(RegisterType::HL), value);
  }
  else
  {
    cpu->setRegister16(cpu->state.instruction.reg1, value);
    value = cpu->readRegister16(cpu->state.instruction.reg1);
  }

  if ((cpu->state.opcode & 0x03) == 0x03)
  {
    return;
  }

  cpu->setFlags(value == 0, 0, (value & 0x0F) == 0, -1);
}

void InstructionsExecuter::sub()
{
  uint16_t value = cpu->readRegister16(cpu->state.instruction.reg1) - cpu->state.opValue;

  int z = value == 0;
  int h = ((int)cpu->readRegister16(cpu->state.instruction.reg1) & NIBBLE_MASK) - ((int)cpu->state.opValue & NIBBLE_MASK) < 0;
  int c = ((int)cpu->readRegister16(cpu->state.instruction.reg1)) - ((int)cpu->state.opValue) < 0;

  cpu->setRegister16(cpu->state.instruction.reg1, value);
  cpu->setFlags(z, 1, h, c);
}

void InstructionsExecuter::sbc()
{
  uint8_t value = cpu->state.opValue + cpu->FLAG_C();

  int z = cpu->readRegister16(cpu->state.instruction.reg1) - value == 0;
  int h = ((int)cpu->readRegister16(cpu->state.instruction.reg1) & NIBBLE_MASK) - ((int)cpu->state.opValue & NIBBLE_MASK) - ((int)cpu->FLAG_C()) < 0;
  int c = ((int)cpu->readRegister16(cpu->state.instruction.reg1)) - ((int)cpu->state.opValue) - ((int)cpu->FLAG_C()) < 0;

  cpu->setRegister16(cpu->state.instruction.reg1, cpu->readRegister16(cpu->state.instruction.reg1) - value);
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

/*
<---BITWISE-START--->
*/

void InstructionsExecuter::bitAnd()
{
  cpu->state.registers.a &= cpu->state.opValue;

  cpu->setFlags(cpu->state.registers.a == 0, 0, 1, 0);
}

void InstructionsExecuter::bitOr()
{
  cpu->state.registers.a |= cpu->state.opValue & BYTE_MASK;

  cpu->setFlags(cpu->state.registers.a == 0, 0, 0, 0);
}

void InstructionsExecuter::bitXor()
{
  cpu->state.registers.a ^= cpu->state.opValue & BYTE_MASK;
  cpu->setFlags(cpu->state.registers.a == 0, 0, 0, 0);
}

void InstructionsExecuter::bitCpl()
{
  cpu->state.registers.a = ~cpu->state.registers.a;
  cpu->setFlags(-1, 1, 1, -1);
}

void InstructionsExecuter::bitCb()
{
  uint8_t opcode = cpu->state.opValue;
  RegisterType reg = cpu->decodeRegister(opcode & 0b111);
  uint8_t bit = (opcode >> 3) & 0b111;
  uint8_t bitOpcode = (opcode >> 6) & 0b11;
  uint8_t registerValue = cpu->readRegister8(reg);

  cpu->cycleCallback(1);

  if (reg == RegisterType::HL)
  {
    cpu->cycleCallback(2);
  }

  switch (bitOpcode)
  {
  case 1:
    cpu->setFlags(!(registerValue & (1 << bit)), 0, 1, -1);
    return;
  case 2:
    registerValue &= ~(1 << bit);
    cpu->setRegister8(reg, registerValue);
    return;
  case 3:
    registerValue |= (1 << bit);
    cpu->setRegister8(reg, registerValue);
    return;
  }

  bool flagc = cpu->FLAG_C();

  switch (bit)
  {
  case 0:
  {
    bool setC = false;
    uint8_t result = (registerValue << 1) & BYTE_MASK;

    if ((registerValue & (1 << 7)) != 0)
    {
      result |= 1;
      setC = true;
    }

    cpu->setRegister8(reg, result);
    cpu->setFlags(result == 0, false, false, setC);
  }
    return;

  case 1:
  {
    uint8_t old = registerValue;
    registerValue >>= 1;
    registerValue |= (old << 7);
    cpu->setRegister8(reg, registerValue);
    cpu->setFlags(!registerValue, false, false, old & 1);
  }
    return;

  case 2:
  {
    uint8_t old = registerValue;
    registerValue <<= 1;
    registerValue |= flagc;

    cpu->setRegister8(reg, registerValue);
    cpu->setFlags(!registerValue, false, false, !!(old & 0x80));
  }
    return;

  case 3:
  {
    uint8_t old = registerValue;
    registerValue >>= 1;
    registerValue |= (flagc << 7);
    cpu->setRegister8(reg, registerValue);
    cpu->setFlags(!registerValue, false, false, old & 1);
  }
    return;

  case 4:
  {
    uint8_t old = registerValue;
    registerValue <<= 1;

    cpu->setRegister8(reg, registerValue);
    cpu->setFlags(!registerValue, false, false, !!(old & 0x80));
  }
    return;

  case 5:
  {
    uint8_t u = (int8_t)registerValue >> 1;
    cpu->setRegister8(reg, u);
    cpu->setFlags(!u, 0, 0, registerValue & 1);
  }
    return;

  case 6:
  {
    registerValue = ((registerValue & 0xF0) >> 4) | ((registerValue & 0xF) << 4);
    cpu->setRegister8(reg, registerValue);
    cpu->setFlags(registerValue == 0, false, false, false);
  }
    return;

  case 7:
  {
    uint8_t u = registerValue >> 1;
    cpu->setRegister8(reg, u);
    cpu->setFlags(!u, 0, 0, registerValue & 1);
  }
    return;
  }
}

/*
<---BITWISE-END--->
*/

/*
<---JUMP-START--->
*/

void InstructionsExecuter::jp()
{
  cpu->jumpToAddress(cpu->state.opValue, false);
}

void InstructionsExecuter::jr()
{
  int8_t rel = (int8_t)(cpu->state.opValue & BYTE_MASK);
  uint16_t updatedPC = cpu->state.registers.pc + rel;
  cpu->jumpToAddress(updatedPC, false);
}

void InstructionsExecuter::call()
{
  cpu->jumpToAddress(cpu->state.opValue, true);
}

/*
<---JUMP-END--->
*/

/*
<---STACK-START--->
*/

void InstructionsExecuter::pop()
{
  uint16_t low = cpu->stackPop8();
  cpu->cycleCallback(1);
  uint16_t high = cpu->stackPop8();
  cpu->cycleCallback(1);

  uint16_t n = (high << 8) | low;

  cpu->setRegister16(cpu->state.instruction.reg1, n);

  if (cpu->state.instruction.reg1 == RegisterType::AF)
  {
    cpu->setRegister16(cpu->state.instruction.reg1, n & 0xFFF0);
  }
}

void InstructionsExecuter::push()
{
  uint16_t high = (cpu->readRegister16(cpu->state.instruction.reg1) >> 8) & BYTE_MASK;
  cpu->cycleCallback(1);
  cpu->stackPush8(high);
  uint16_t low = cpu->readRegister16(cpu->state.instruction.reg1) & BYTE_MASK;
  cpu->cycleCallback(1);
  cpu->stackPush8(low);
  cpu->cycleCallback(1);
}

/*
<---STACK-END--->
*/
/*
<---BIT-SHIFT-START--->
*/

void InstructionsExecuter::rlca()
{
  uint8_t u = cpu->state.registers.a;
  bool c = (u >> 7) & 1;
  u = (u << 1) | c;
  cpu->state.registers.a = u;

  cpu->setFlags(0, 0, 0, c);
}

void InstructionsExecuter::rrca()
{
  uint8_t b = cpu->state.registers.a & 1;
  cpu->state.registers.a >>= 1;
  cpu->state.registers.a |= (b << 7);

  cpu->setFlags(0, 0, 0, b);
}

void InstructionsExecuter::rla()
{
  uint8_t u = cpu->state.registers.a;
  uint8_t cf = cpu->FLAG_C();
  uint8_t c = (u >> 7) & 1;

  cpu->state.registers.a = (u << 1) | cf;
  cpu->setFlags(0, 0, 0, c);
}

void InstructionsExecuter::rra()
{
  uint8_t carry = cpu->FLAG_C();
  uint8_t new_c = cpu->state.registers.a & 1;

  cpu->state.registers.a >>= 1;
  cpu->state.registers.a |= (carry << 7);

  cpu->setFlags(0, 0, 0, new_c);
}

void InstructionsExecuter::daa()
{
  uint8_t u = 0;
  int c = 0;

  if (cpu->FLAG_H() || (!cpu->FLAG_N() && (cpu->state.registers.a & 0xF) > 9))
  {
    u = 6;
  }

  if (cpu->FLAG_C() || (!cpu->FLAG_N() && cpu->state.registers.a > 0x99))
  {
    u |= 0x60;
    c = 1;
  }

  cpu->state.registers.a += cpu->FLAG_N() ? -u : u;

  cpu->setFlags(cpu->state.registers.a == 0, -1, 0, c);
}

/*
<---BIT-SHIFT-END--->
*/

/*
<---BIT-FLAG-START--->
*/

void InstructionsExecuter::scf()
{
  cpu->setFlags(-1, 0, 0, 1);
}

void InstructionsExecuter::ccf()
{
  cpu->setFlags(-1, 0, 0, cpu->FLAG_C() ^ 1);
}

/*
<---BIT-FLAG-END--->
*/

/*
<---INTERRUPT-START--->
*/

void InstructionsExecuter::di()
{
  cpu->state.ime = false;
}

void InstructionsExecuter::ei()
{
  cpu->state.imeScheduled = true;
}

/*
<---INTERRUPT-END--->
*/