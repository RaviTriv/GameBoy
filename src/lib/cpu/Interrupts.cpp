#include "Interrupts.h"
#include "CpuContext.h"
#include "Logger.h"

Interrupts::Interrupts(CpuContext *ctx) : ctx(ctx) {}

void Interrupts::interruptHandle(uint16_t address)
{
  ctx->stackPush16(ctx->state.registers.pc);
  ctx->setRegister16(RegisterType::PC, address);
}

bool Interrupts::checkInterrupt(uint16_t address, InterruptType type)
{
  if (ctx->state.intf & static_cast<int>(type) && ctx->state.ie & static_cast<int>(type))
  {
    interruptHandle(address);
    ctx->state.intf &= ~static_cast<int>(type);
    ctx->state.halted = false;
    ctx->state.ime = false;

    return true;
  }

  return false;
}

void Interrupts::handleInterrupts()
{
  if (checkInterrupt(0x40, InterruptType::VBLANK))
  {
  }
  else if (checkInterrupt(0x48, InterruptType::LCD_STAT))
  {
  }
  else if (checkInterrupt(0x50, InterruptType::TIMER))
  {
  }
  else if (checkInterrupt(0x58, InterruptType::SERIAL))
  {
  }
  else if (checkInterrupt(0x60, InterruptType::JOYPAD))
  {
  }
}

void Interrupts::requestInterrupt(InterruptType type)
{
  ctx->state.intf |= (uint8_t)type;
}

void Interrupts::setInterruptEnable(uint8_t value)
{
  ctx->state.ie = value;
}

uint8_t Interrupts::getInterruptEnable() const
{
  return ctx->state.ie;
}
