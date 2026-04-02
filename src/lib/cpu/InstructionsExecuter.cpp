#include "InstructionsExecuter.h"
#include "Bus.h"
#include "CpuContext.h"
#include "Logger.h"

InstructionsExecuter::InstructionsExecuter(CpuContext *ctx) : ctx(ctx) {}

void InstructionsExecuter::execute()
{
  switch (ctx->state.instruction.type)
  {
  case InstructionType::NOP:
    ctx->cycle(1);
    break;
  case InstructionType::XOR:
    bitXor();
    break;
  case InstructionType::JP:
    jp();
    break;
  case InstructionType::DI:
    di();
    break;
  case InstructionType::JR:
    jr();
    break;
  case InstructionType::CALL:
    call();
    break;
  case InstructionType::LD:
    ld();
    break;
  case InstructionType::LDH:
    ldh();
    break;
  case InstructionType::POP:
    pop();
    break;
  case InstructionType::PUSH:
    push();
    break;
  case InstructionType::RET:
    ret();
    break;
  case InstructionType::RETI:
    reti();
    break;
  case InstructionType::RST:
    rst();
    break;
  case InstructionType::INC:
    inc();
    break;
  case InstructionType::DEC:
    dec();
    break;
  case InstructionType::ADD:
    add();
    break;
  case InstructionType::ADC:
    adc();
    break;
  case InstructionType::SUB:
    sub();
    break;
  case InstructionType::SBC:
    sbc();
    break;
  case InstructionType::AND:
    bitAnd();
    break;
  case InstructionType::OR:
    bitOr();
    break;
  case InstructionType::CP:
    cp();
    break;
  case InstructionType::RLC:
    cbRlc();
    break;
  case InstructionType::RRC:
    cbRrc();
    break;
  case InstructionType::RL:
    cbRl();
    break;
  case InstructionType::RR:
    cbRr();
    break;
  case InstructionType::SLA:
    cbSla();
    break;
  case InstructionType::SRA:
    cbSra();
    break;
  case InstructionType::SWAP:
    cbSwap();
    break;
  case InstructionType::SRL:
    cbSrl();
    break;
  case InstructionType::BIT:
    cbBit();
    break;
  case InstructionType::RES:
    cbRes();
    break;
  case InstructionType::SET:
    cbSet();
    break;
  case InstructionType::RLCA:
    rlca();
    break;
  case InstructionType::RRCA:
    rrca();
    break;
  case InstructionType::RLA:
    rla();
    break;
  case InstructionType::RRA:
    rra();
    break;
  case InstructionType::DAA:
    daa();
    break;
  case InstructionType::CPL:
    bitCpl();
    break;
  case InstructionType::SCF:
    scf();
    break;
  case InstructionType::CCF:
    ccf();
    break;
  case InstructionType::EI:
    ei();
    break;
  case InstructionType::HALT:
    halt();
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
  uint32_t value = ctx->readRegister16(ctx->state.instruction.reg1) + ctx->state.opValue;
  bool is16BIT = ctx->is16Bit(ctx->state.instruction.reg1);

  if (is16BIT)
  {
    ctx->cycle(1);
  }

  if (ctx->state.instruction.reg1 == RegisterType::SP)
  {
    value = ctx->readRegister16(ctx->state.instruction.reg1) + (int8_t)ctx->state.opValue;
  }

  int z = (value & BYTE_MASK) == 0;
  int h = (ctx->readRegister16(ctx->state.instruction.reg1) & NIBBLE_MASK) + (ctx->state.opValue & NIBBLE_MASK) >= 0x10;
  int c = (int)(ctx->readRegister16(ctx->state.instruction.reg1) & BYTE_MASK) + (int)(ctx->state.opValue & BYTE_MASK) >= 0x100;

  if (is16BIT)
  {
    z = -1;
    h = (ctx->readRegister16(ctx->state.instruction.reg1) & 0xFFF) + (ctx->state.opValue & 0xFFF) >= 0x1000;
    uint32_t n = ((uint32_t)ctx->readRegister16(ctx->state.instruction.reg1)) + ((uint32_t)ctx->state.opValue);
    c = n >= 0x10000;
  }

  if (ctx->state.instruction.reg1 == RegisterType::SP)
  {
    z = 0;
    h = (ctx->readRegister16(ctx->state.instruction.reg1) & NIBBLE_MASK) + (ctx->state.opValue & NIBBLE_MASK) >= 0x10;
    c = (int)(ctx->readRegister16(ctx->state.instruction.reg1) & BYTE_MASK) + (int)(ctx->state.opValue & BYTE_MASK) >= 0x100;
  }

  ctx->setRegister16(ctx->state.instruction.reg1, value & 0xFFFF);
  ctx->setFlags(z, 0, h, c);
}

void InstructionsExecuter::adc()
{
  uint16_t val = ctx->state.opValue;
  uint16_t a = ctx->state.registers.a;
  uint16_t c = ctx->FLAG_C();

  ctx->state.registers.a = (a + val + c) & BYTE_MASK;

  ctx->setFlags(ctx->state.registers.a == 0, 0,
                (a & NIBBLE_MASK) + (val & NIBBLE_MASK) + c > NIBBLE_MASK,
                a + val + c > BYTE_MASK);
}

void InstructionsExecuter::dec()
{
  uint16_t value = ctx->readRegister16(ctx->state.instruction.reg1) - 1;

  if (ctx->is16Bit(ctx->state.instruction.reg1))
  {
    ctx->cycle(1);
  }

  if (ctx->state.instruction.reg1 == RegisterType::HL && ctx->state.instruction.addressMode == AddressingMode::MR)
  {
    value = ctx->bus->read8(ctx->readRegister16(RegisterType::HL)) - 1;
    ctx->bus->write8(ctx->readRegister16(RegisterType::HL), value);
  }
  else
  {
    ctx->setRegister16(ctx->state.instruction.reg1, value);
    value = ctx->readRegister16(ctx->state.instruction.reg1);
  }

  if ((ctx->state.opcode & 0x0B) == 0x0B)
  {
    return;
  }

  ctx->setFlags(value == 0, 1, (value & 0x0F) == 0x0F, -1);
}

void InstructionsExecuter::inc()
{
  uint16_t value = ctx->readRegister16(ctx->state.instruction.reg1) + 1;

  if (ctx->is16Bit(ctx->state.instruction.reg1))
  {
    ctx->cycle(1);
  }

  if (ctx->state.instruction.reg1 == RegisterType::HL && ctx->state.instruction.addressMode == AddressingMode::MR)
  {
    value = ctx->bus->read8(ctx->readRegister16(RegisterType::HL)) + 1;
    value &= BYTE_MASK;
    ctx->bus->write8(ctx->readRegister16(RegisterType::HL), value);
  }
  else
  {
    ctx->setRegister16(ctx->state.instruction.reg1, value);
    value = ctx->readRegister16(ctx->state.instruction.reg1);
  }

  if ((ctx->state.opcode & 0x03) == 0x03)
  {
    return;
  }

  ctx->setFlags(value == 0, 0, (value & 0x0F) == 0, -1);
}

void InstructionsExecuter::sub()
{
  uint16_t value = ctx->readRegister16(ctx->state.instruction.reg1) - ctx->state.opValue;

  int z = value == 0;
  int h = ((int)ctx->readRegister16(ctx->state.instruction.reg1) & NIBBLE_MASK) - ((int)ctx->state.opValue & NIBBLE_MASK) < 0;
  int c = ((int)ctx->readRegister16(ctx->state.instruction.reg1)) - ((int)ctx->state.opValue) < 0;

  ctx->setRegister16(ctx->state.instruction.reg1, value);
  ctx->setFlags(z, 1, h, c);
}

void InstructionsExecuter::sbc()
{
  uint8_t value = ctx->state.opValue + ctx->FLAG_C();

  int z = ctx->readRegister16(ctx->state.instruction.reg1) - value == 0;
  int h = ((int)ctx->readRegister16(ctx->state.instruction.reg1) & NIBBLE_MASK) - ((int)ctx->state.opValue & NIBBLE_MASK) - ((int)ctx->FLAG_C()) < 0;
  int c = ((int)ctx->readRegister16(ctx->state.instruction.reg1)) - ((int)ctx->state.opValue) - ((int)ctx->FLAG_C()) < 0;

  ctx->setRegister16(ctx->state.instruction.reg1, ctx->readRegister16(ctx->state.instruction.reg1) - value);
  ctx->setFlags(z, 1, h, c);
}

void InstructionsExecuter::cp()
{
  int tmp = (int)ctx->state.registers.a - (int)ctx->state.opValue;
  ctx->setFlags(tmp == 0, 1, ((int)ctx->state.registers.a & 0x0F) - ((int)ctx->state.opValue & 0x0F) < 0, tmp < 0);
}
/*
<---ARITHMETIC-END--->
*/

/*
<---LOAD-START--->
*/

void InstructionsExecuter::ld()
{
  if (ctx->state.isMemoryOp)
  {
    if (ctx->is16Bit(ctx->state.instruction.reg2))
    {
      ctx->cycle(1);
      ctx->bus->write16(ctx->state.memoryAddress, ctx->state.opValue);
    }
    else
    {
      ctx->bus->write8(ctx->state.memoryAddress, ctx->state.opValue);
    }
    ctx->cycle(1);
    return;
  }

  if (ctx->state.instruction.addressMode == AddressingMode::HL_SPR)
  {
    uint8_t H =
        (ctx->readRegister16(ctx->state.instruction.reg2) & 0xF) + (ctx->state.opValue & 0xF) >= 0x10;
    uint8_t C = (ctx->readRegister16(ctx->state.instruction.reg2) & BYTE_MASK) +
                    (ctx->state.opValue & BYTE_MASK) >=
                0x100;
    ctx->setFlags(0, 0, H, C);
    ctx->setRegister16(ctx->state.instruction.reg1, ctx->readRegister16(ctx->state.instruction.reg2) + (char)ctx->state.opValue);
    return;
  }
  ctx->setRegister16(ctx->state.instruction.reg1, ctx->state.opValue);
}

void InstructionsExecuter::ldh()
{
  if (ctx->state.instruction.reg1 == RegisterType::A)
  {
    ctx->setRegister16(RegisterType::A, ctx->bus->read8(0xFF00 | ctx->state.opValue));
  }
  else
  {
    ctx->bus->write8(ctx->state.memoryAddress, ctx->state.registers.a);
  }
  ctx->cycle(1);
}

/*
<---LOAD-END--->
*/

/*
<---BITWISE-START--->
*/

void InstructionsExecuter::bitAnd()
{
  ctx->state.registers.a &= ctx->state.opValue;

  ctx->setFlags(ctx->state.registers.a == 0, 0, 1, 0);
}

void InstructionsExecuter::bitOr()
{
  ctx->state.registers.a |= ctx->state.opValue & BYTE_MASK;

  ctx->setFlags(ctx->state.registers.a == 0, 0, 0, 0);
}

void InstructionsExecuter::bitXor()
{
  ctx->state.registers.a ^= ctx->state.opValue & BYTE_MASK;
  ctx->setFlags(ctx->state.registers.a == 0, 0, 0, 0);
}

void InstructionsExecuter::bitCpl()
{
  ctx->state.registers.a = ~ctx->state.registers.a;
  ctx->setFlags(-1, 1, 1, -1);
}

void InstructionsExecuter::cbRlc()
{
  RegisterType reg = ctx->state.instruction.reg1;
  uint8_t registerValue = ctx->readRegister8(reg);

  ctx->cycle(1);
  if (reg == RegisterType::HL) ctx->cycle(2);

  bool setC = (registerValue & (1 << 7)) != 0;
  uint8_t result = (registerValue << 1) & BYTE_MASK;
  if (setC) result |= 1;

  ctx->setRegister8(reg, result);
  ctx->setFlags(result == 0, false, false, setC);
}

void InstructionsExecuter::cbRrc()
{
  RegisterType reg = ctx->state.instruction.reg1;
  uint8_t registerValue = ctx->readRegister8(reg);

  ctx->cycle(1);
  if (reg == RegisterType::HL) ctx->cycle(2);

  uint8_t old = registerValue;
  registerValue >>= 1;
  registerValue |= (old << 7);

  ctx->setRegister8(reg, registerValue);
  ctx->setFlags(!registerValue, false, false, old & 1);
}

void InstructionsExecuter::cbRl()
{
  RegisterType reg = ctx->state.instruction.reg1;
  uint8_t registerValue = ctx->readRegister8(reg);

  ctx->cycle(1);
  if (reg == RegisterType::HL) ctx->cycle(2);

  uint8_t old = registerValue;
  registerValue <<= 1;
  registerValue |= ctx->FLAG_C();

  ctx->setRegister8(reg, registerValue);
  ctx->setFlags(!registerValue, false, false, !!(old & 0x80));
}

void InstructionsExecuter::cbRr()
{
  RegisterType reg = ctx->state.instruction.reg1;
  uint8_t registerValue = ctx->readRegister8(reg);

  ctx->cycle(1);
  if (reg == RegisterType::HL) ctx->cycle(2);

  uint8_t old = registerValue;
  registerValue >>= 1;
  registerValue |= (ctx->FLAG_C() << 7);

  ctx->setRegister8(reg, registerValue);
  ctx->setFlags(!registerValue, false, false, old & 1);
}

void InstructionsExecuter::cbSla()
{
  RegisterType reg = ctx->state.instruction.reg1;
  uint8_t registerValue = ctx->readRegister8(reg);

  ctx->cycle(1);
  if (reg == RegisterType::HL) ctx->cycle(2);

  uint8_t old = registerValue;
  registerValue <<= 1;

  ctx->setRegister8(reg, registerValue);
  ctx->setFlags(!registerValue, false, false, !!(old & 0x80));
}

void InstructionsExecuter::cbSra()
{
  RegisterType reg = ctx->state.instruction.reg1;
  uint8_t registerValue = ctx->readRegister8(reg);

  ctx->cycle(1);
  if (reg == RegisterType::HL) ctx->cycle(2);

  uint8_t u = (int8_t)registerValue >> 1;
  ctx->setRegister8(reg, u);
  ctx->setFlags(!u, 0, 0, registerValue & 1);
}

void InstructionsExecuter::cbSwap()
{
  RegisterType reg = ctx->state.instruction.reg1;
  uint8_t registerValue = ctx->readRegister8(reg);

  ctx->cycle(1);
  if (reg == RegisterType::HL) ctx->cycle(2);

  registerValue = ((registerValue & 0xF0) >> 4) | ((registerValue & 0xF) << 4);
  ctx->setRegister8(reg, registerValue);
  ctx->setFlags(registerValue == 0, false, false, false);
}

void InstructionsExecuter::cbSrl()
{
  RegisterType reg = ctx->state.instruction.reg1;
  uint8_t registerValue = ctx->readRegister8(reg);

  ctx->cycle(1);
  if (reg == RegisterType::HL) ctx->cycle(2);

  uint8_t u = registerValue >> 1;
  ctx->setRegister8(reg, u);
  ctx->setFlags(!u, 0, 0, registerValue & 1);
}

void InstructionsExecuter::cbBit()
{
  RegisterType reg = ctx->state.instruction.reg1;
  uint8_t registerValue = ctx->readRegister8(reg);
  uint8_t bit = ctx->state.instruction.parameter;

  ctx->cycle(1);
  if (reg == RegisterType::HL) ctx->cycle(2);

  ctx->setFlags(!(registerValue & (1 << bit)), 0, 1, -1);
}

void InstructionsExecuter::cbRes()
{
  RegisterType reg = ctx->state.instruction.reg1;
  uint8_t registerValue = ctx->readRegister8(reg);
  uint8_t bit = ctx->state.instruction.parameter;

  ctx->cycle(1);
  if (reg == RegisterType::HL) ctx->cycle(2);

  registerValue &= ~(1 << bit);
  ctx->setRegister8(reg, registerValue);
}

void InstructionsExecuter::cbSet()
{
  RegisterType reg = ctx->state.instruction.reg1;
  uint8_t registerValue = ctx->readRegister8(reg);
  uint8_t bit = ctx->state.instruction.parameter;

  ctx->cycle(1);
  if (reg == RegisterType::HL) ctx->cycle(2);

  registerValue |= (1 << bit);
  ctx->setRegister8(reg, registerValue);
}

/*
<---BITWISE-END--->
*/

/*
<---JUMP-START--->
*/

void InstructionsExecuter::jp()
{
  ctx->jumpToAddress(ctx->state.opValue, false);
}

void InstructionsExecuter::jr()
{
  int8_t rel = (int8_t)(ctx->state.opValue & BYTE_MASK);
  uint16_t updatedPC = ctx->state.registers.pc + rel;
  ctx->jumpToAddress(updatedPC, false);
}

void InstructionsExecuter::call()
{
  ctx->jumpToAddress(ctx->state.opValue, true);
}

/*
<---JUMP-END--->
*/

/*
<---STACK-START--->
*/

void InstructionsExecuter::pop()
{
  uint16_t low = ctx->stackPop8();
  ctx->cycle(1);
  uint16_t high = ctx->stackPop8();
  ctx->cycle(1);

  uint16_t n = (high << 8) | low;

  ctx->setRegister16(ctx->state.instruction.reg1, n);

  if (ctx->state.instruction.reg1 == RegisterType::AF)
  {
    ctx->setRegister16(ctx->state.instruction.reg1, n & 0xFFF0);
  }
}

void InstructionsExecuter::push()
{
  uint16_t high = (ctx->readRegister16(ctx->state.instruction.reg1) >> 8) & BYTE_MASK;
  ctx->cycle(1);
  ctx->stackPush8(high);
  uint16_t low = ctx->readRegister16(ctx->state.instruction.reg1) & BYTE_MASK;
  ctx->cycle(1);
  ctx->stackPush8(low);
  ctx->cycle(1);
}

/*
<---STACK-END--->
*/
/*
<---BIT-SHIFT-START--->
*/

void InstructionsExecuter::rlca()
{
  uint8_t u = ctx->state.registers.a;
  bool c = (u >> 7) & 1;
  u = (u << 1) | c;
  ctx->state.registers.a = u;

  ctx->setFlags(0, 0, 0, c);
}

void InstructionsExecuter::rrca()
{
  uint8_t b = ctx->state.registers.a & 1;
  ctx->state.registers.a >>= 1;
  ctx->state.registers.a |= (b << 7);

  ctx->setFlags(0, 0, 0, b);
}

void InstructionsExecuter::rla()
{
  uint8_t u = ctx->state.registers.a;
  uint8_t cf = ctx->FLAG_C();
  uint8_t c = (u >> 7) & 1;

  ctx->state.registers.a = (u << 1) | cf;
  ctx->setFlags(0, 0, 0, c);
}

void InstructionsExecuter::rra()
{
  uint8_t carry = ctx->FLAG_C();
  uint8_t new_c = ctx->state.registers.a & 1;

  ctx->state.registers.a >>= 1;
  ctx->state.registers.a |= (carry << 7);

  ctx->setFlags(0, 0, 0, new_c);
}

void InstructionsExecuter::daa()
{
  uint8_t u = 0;
  int c = 0;

  if (ctx->FLAG_H() || (!ctx->FLAG_N() && (ctx->state.registers.a & 0xF) > 9))
  {
    u = 6;
  }

  if (ctx->FLAG_C() || (!ctx->FLAG_N() && ctx->state.registers.a > 0x99))
  {
    u |= 0x60;
    c = 1;
  }

  ctx->state.registers.a += ctx->FLAG_N() ? -u : u;

  ctx->setFlags(ctx->state.registers.a == 0, -1, 0, c);
}

/*
<---BIT-SHIFT-END--->
*/

/*
<---BIT-FLAG-START--->
*/

void InstructionsExecuter::scf()
{
  ctx->setFlags(-1, 0, 0, 1);
}

void InstructionsExecuter::ccf()
{
  ctx->setFlags(-1, 0, 0, ctx->FLAG_C() ^ 1);
}

/*
<---BIT-FLAG-END--->
*/

/*
<---INTERRUPT-START--->
*/

void InstructionsExecuter::di()
{
  ctx->state.ime = false;
}

void InstructionsExecuter::ei()
{
  ctx->state.imeScheduled = true;
}

/*
<---INTERRUPT-END--->
*/

/*
<---CONTROL-START--->
*/

void InstructionsExecuter::ret()
{
  if (ctx->state.instruction.condition != ConditionType::NONE)
  {
    ctx->cycle(1);
  }

  if (ctx->conditionCheck())
  {
    uint16_t low = ctx->stackPop8();
    ctx->cycle(1);
    uint16_t high = ctx->stackPop8();
    ctx->cycle(1);

    uint16_t res = (high << 8) | low;
    ctx->state.registers.pc = res;

    ctx->cycle(1);
  }
}

void InstructionsExecuter::reti()
{
  ctx->state.ime = true;
  ret();
}

void InstructionsExecuter::rst()
{
  ctx->jumpToAddress(ctx->state.instruction.parameter, true);
}

void InstructionsExecuter::halt()
{
  ctx->state.halted = true;
}

/*
<---CONTROL-END--->
*/
