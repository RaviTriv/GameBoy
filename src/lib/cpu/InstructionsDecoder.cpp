#include "InstructionsDecoder.h"
#include "Bus.h"
#include "CpuContext.h"
#include "Logger.h"

InstructionsDecoder::InstructionsDecoder(CpuContext *ctx) : ctx(ctx) {}

const Instruction &InstructionsDecoder::getInstruction(uint8_t opcode)
{
  return INSTRUCTIONS[opcode];
}

InstructionsDecoder::AddressModeHandler InstructionsDecoder::addressModeHandlers[] = {
    &InstructionsDecoder::none,
    &InstructionsDecoder::imp,
    &InstructionsDecoder::r_d16,
    &InstructionsDecoder::r_r,
    &InstructionsDecoder::mr_r,
    &InstructionsDecoder::r,
    &InstructionsDecoder::r_d8,
    &InstructionsDecoder::r_mr,
    &InstructionsDecoder::r_hli,
    &InstructionsDecoder::r_hld,
    &InstructionsDecoder::hli_r,
    &InstructionsDecoder::hld_r,
    &InstructionsDecoder::r_a8,
    &InstructionsDecoder::a8_r,
    &InstructionsDecoder::hl_spr,
    &InstructionsDecoder::r_d16,
    &InstructionsDecoder::d8,
    &InstructionsDecoder::a16_r,
    &InstructionsDecoder::mr_d8,
    &InstructionsDecoder::mr,
    &InstructionsDecoder::a16_r,
    &InstructionsDecoder::r_a16
};

void InstructionsDecoder::decode(uint8_t opcode)
{
  const Instruction &instruction = getInstruction(opcode);
  ctx->state.instruction = instruction;

  ctx->cycle(1);

  ctx->state.memoryAddress = 0;
  ctx->state.isMemoryOp = false;

  (this->*addressModeHandlers[static_cast<int>(instruction.addressMode)])();
}

void InstructionsDecoder::none()
{
}

void InstructionsDecoder::imp()
{
}

void InstructionsDecoder::r()
{
  ctx->state.opValue = ctx->readRegister16(ctx->state.instruction.reg1);
}

void InstructionsDecoder::r_r()
{
  ctx->state.opValue = ctx->readRegister16(ctx->state.instruction.reg2);
}

void InstructionsDecoder::r_d8()
{
  ctx->state.opValue = ctx->bus->read8(ctx->state.registers.pc);
  ctx->cycle(1);
  ctx->state.registers.pc++;
}
void InstructionsDecoder::r_d16()
{
  uint16_t low = ctx->bus->read8(ctx->state.registers.pc);
  ctx->cycle(1);
  uint16_t high = ctx->bus->read8(ctx->state.registers.pc + 1);
  ctx->cycle(1);
  ctx->state.opValue = low | (high << 8);
  ctx->state.registers.pc += 2;
}

void InstructionsDecoder::mr_r()
{
  ctx->state.memoryAddress = ctx->readRegister16(ctx->state.instruction.reg1);
  ctx->state.isMemoryOp = true;

  ctx->state.opValue = ctx->readRegister16(ctx->state.instruction.reg2);

  if (ctx->state.instruction.reg1 == RegisterType::C)
  {
    ctx->state.memoryAddress |= 0xFF00;
  }
}

void InstructionsDecoder::r_mr()
{
  uint16_t address = ctx->readRegister16(ctx->state.instruction.reg2);

  if (ctx->state.instruction.reg2 == RegisterType::C)
  {
    address |= 0xFF00;
  }

  ctx->state.opValue = ctx->bus->read8(address);
  ctx->cycle(1);
}

void InstructionsDecoder::r_hli()
{
  ctx->state.opValue = ctx->bus->read8(ctx->readRegister16(ctx->state.instruction.reg2));
  ctx->cycle(1);
  ctx->setRegister16(RegisterType::HL, ctx->readRegister16(RegisterType::HL) + 1);
}

void InstructionsDecoder::r_hld()
{
  ctx->state.opValue = ctx->bus->read8(ctx->readRegister16(ctx->state.instruction.reg2));
  ctx->cycle(1);
  ctx->setRegister16(RegisterType::HL, ctx->readRegister16(RegisterType::HL) - 1);
}

void InstructionsDecoder::hli_r()
{
  ctx->state.memoryAddress = ctx->readRegister16(ctx->state.instruction.reg1);
  ctx->state.isMemoryOp = true;

  ctx->state.opValue = ctx->readRegister16(ctx->state.instruction.reg2);
  ctx->setRegister16(RegisterType::HL, ctx->readRegister16(RegisterType::HL) + 1);
}

void InstructionsDecoder::hld_r()
{
  ctx->state.memoryAddress = ctx->readRegister16(ctx->state.instruction.reg1);
  ctx->state.isMemoryOp = true;

  ctx->state.opValue = ctx->readRegister16(ctx->state.instruction.reg2);
  ctx->setRegister16(RegisterType::HL, ctx->readRegister16(RegisterType::HL) - 1);
}

void InstructionsDecoder::r_a8()
{
  ctx->state.opValue = ctx->bus->read8((ctx->state.registers.pc));
  ctx->cycle(1);
  ctx->state.registers.pc++;
}

void InstructionsDecoder::a8_r()
{
  ctx->state.memoryAddress = ctx->bus->read8(ctx->state.registers.pc) | 0xFF00;
  ctx->state.isMemoryOp = true;
  ctx->cycle(1);
  ctx->state.registers.pc++;
}

void InstructionsDecoder::d8()
{
  ctx->state.opValue = ctx->bus->read8(ctx->state.registers.pc);
  ctx->cycle(1);
  ctx->state.registers.pc++;
}

void InstructionsDecoder::hl_spr()
{
  ctx->state.opValue = ctx->bus->read8(ctx->state.registers.pc);
  ctx->cycle(1);
  ctx->state.registers.pc++;
}

void InstructionsDecoder::a16_r()
{
  uint16_t low = ctx->bus->read8(ctx->state.registers.pc);
  ctx->cycle(1);
  uint16_t high = ctx->bus->read8(ctx->state.registers.pc + 1);
  ctx->cycle(1);
  ctx->state.memoryAddress = low | (high << 8);
  ctx->state.isMemoryOp = true;
  ctx->state.registers.pc += 2;
  ctx->state.opValue = ctx->readRegister16(ctx->state.instruction.reg2);
}

void InstructionsDecoder::mr_d8()
{
  ctx->state.opValue = ctx->bus->read8(ctx->state.registers.pc);
  ctx->cycle(1);
  ctx->state.registers.pc++;
  ctx->state.memoryAddress = ctx->readRegister16(ctx->state.instruction.reg1);
  ctx->state.isMemoryOp = true;
}

void InstructionsDecoder::mr()
{
  ctx->state.memoryAddress = ctx->readRegister16(ctx->state.instruction.reg1);
  ctx->state.isMemoryOp = true;
  ctx->state.opValue = ctx->bus->read8(ctx->state.memoryAddress);
  ctx->cycle(1);
}

void InstructionsDecoder::r_a16()
{
  uint16_t low = ctx->bus->read8(ctx->state.registers.pc);
  ctx->cycle(1);
  uint16_t high = ctx->bus->read8(ctx->state.registers.pc + 1);
  ctx->cycle(1);
  ctx->state.opValue = ctx->bus->read8(low | (high << 8));
  ctx->cycle(1);
  ctx->state.registers.pc += 2;
}
