#include "../../../include/Interrupts.h"
#include "../../../include/Cpu.h"
#include "../../../include/Logger.h"

Interrupts::Interrupts(CPU *cpu) : cpu(cpu) {}

void Interrupts::interruptHandle(uint16_t address)
{
  cpu->stackPush16(cpu->state.registers.pc);
  cpu->setRegister16(RegisterType::PC, address);
}

bool Interrupts::checkInterrupt(uint16_t address, InterruptType type)
{
  if (cpu->state.intf & static_cast<int>(type) && cpu->state.ie & static_cast<int>(type))
  {
    interruptHandle(address);
    cpu->state.intf &= ~static_cast<int>(type);
    cpu->state.halted = false;
    cpu->state.ime = false;

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
  cpu->state.intf |= (uint8_t)type;
}

void Interrupts::setInterruptEnable(uint8_t value)
{
  cpu->state.ie = value;
}

uint8_t Interrupts::getInterruptEnable() const
{
  return cpu->state.ie;
}