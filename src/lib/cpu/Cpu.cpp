#include "../../../include/Cpu.h"
#include "../../../include/Bus.h"
#include "../../../include/Logger.h"

RegisterType registerLookup[] = {
    RegisterType::B,
    RegisterType::C,
    RegisterType::D,
    RegisterType::E,
    RegisterType::H,
    RegisterType::L,
    RegisterType::HL,
    RegisterType::A};

CPU::CPU(CycleCallback cycleCallback, std::shared_ptr<Bus> bus) : cycleCallback(cycleCallback), decoder(this), executer(this), interrupt(this), bus(bus)
{
  state.registers.a = 0xB001;
  state.registers.f = 0xB0;
  state.registers.b = 0x1300;
  state.registers.c = 0x13;
  state.registers.d = 0xD800;
  state.registers.e = 0xD8;
  state.registers.h = 0x4D01;
  state.registers.l = 0x4D;
  state.registers.sp = 0xFFFE;
  state.registers.pc = 0x0100;
  state.ie = 0x00;
  state.intf = 0;
  state.imeScheduled = false;
  state.ime = false;
};

void CPU::fetch()
{
  state.opcode = bus->read8(state.registers.pc++);
}

void CPU::decode()
{
  decoder.decode(state.opcode);
}

void CPU::execute()
{
  executer.execute();
}

void CPU::step()
{
  if (!state.halted)
  {
    fetch();
    decode();
    execute();
  }
  else
  {
    cycleCallback(1);
    if (state.intf)
    {
      state.halted = false;
    }
  }

  if (state.ime)
  {
    interrupt.handleInterrupts();
    state.imeScheduled = false;
  }

  if (state.imeScheduled)
  {
    state.ime = true;
  }
}

uint8_t CPU::readRegister8(RegisterType reg) const
{
  switch (reg)
  {
  case RegisterType::A:
    return state.registers.a;
  case RegisterType::B:
    return state.registers.b;
  case RegisterType::C:
    return state.registers.c;
  case RegisterType::D:
    return state.registers.d;
  case RegisterType::E:
    return state.registers.e;
  case RegisterType::F:
    return state.registers.f;
  case RegisterType::H:
    return state.registers.h;
  case RegisterType::L:
    return state.registers.l;
  case RegisterType::HL:
    return bus->read8(readRegister16(RegisterType::HL));
  case RegisterType::NONE:
    throw std::runtime_error("Trying to read invalid CPU register");
  default:
    throw std::runtime_error("Trying to read invalid CPU register");
  }
}

uint16_t CPU::readRegister16(RegisterType reg) const
{
  switch (reg)
  {
  case RegisterType::A:
    return state.registers.a;
  case RegisterType::B:
    return state.registers.b;
  case RegisterType::C:
    return state.registers.c;
  case RegisterType::D:
    return state.registers.d;
  case RegisterType::E:
    return state.registers.e;
  case RegisterType::F:
    return state.registers.f;
  case RegisterType::H:
    return state.registers.h;
  case RegisterType::L:
    return state.registers.l;
  case RegisterType::AF:
    return (state.registers.a << 8) | (state.registers.f & 0xF0);
  case RegisterType::BC:
    return (state.registers.b << 8) | state.registers.c;
  case RegisterType::DE:
    return (state.registers.d << 8) | state.registers.e;
  case RegisterType::HL:
    return (state.registers.h << 8) | state.registers.l;
  case RegisterType::SP:
    return state.registers.sp;
  case RegisterType::PC:
    return state.registers.pc;
  case RegisterType::NONE:
    throw std::runtime_error("Trying to read invalid CPU register");
  default:
    throw std::runtime_error("Trying to read invalid CPU register");
  }
}

void CPU::setRegister8(RegisterType reg, uint8_t value)
{
  switch (reg)
  {
  case RegisterType::A:
    state.registers.a = value & BYTE_MASK;
    break;
  case RegisterType::B:
    state.registers.b = value & BYTE_MASK;
    break;
  case RegisterType::C:
    state.registers.c = value & BYTE_MASK;
    break;
  case RegisterType::D:
    state.registers.d = value & BYTE_MASK;
    break;
  case RegisterType::E:
    state.registers.e = value & BYTE_MASK;
    break;
  case RegisterType::F:
    state.registers.f = value & BYTE_MASK;
    break;
  case RegisterType::H:
    state.registers.h = value & BYTE_MASK;
    break;
  case RegisterType::L:
    state.registers.l = value & BYTE_MASK;
    break;
  case RegisterType::HL:
    bus->write8(readRegister16(RegisterType::HL), value);
    break;
  default:
    throw std::runtime_error("Trying to write to invalid CPU register");
  }
}

void CPU::setRegister16(RegisterType reg, uint16_t value)
{
  switch (reg)
  {
  case RegisterType::A:
    state.registers.a = value & BYTE_MASK;
    break;
  case RegisterType::B:
    state.registers.b = value & BYTE_MASK;
    break;
  case RegisterType::C:
    state.registers.c = value & BYTE_MASK;
    break;
  case RegisterType::D:
    state.registers.d = value & BYTE_MASK;
    break;
  case RegisterType::E:
    state.registers.e = value & BYTE_MASK;
    break;
  case RegisterType::F:
    state.registers.f = value & BYTE_MASK;
    break;
  case RegisterType::H:
    state.registers.h = value & BYTE_MASK;
    break;
  case RegisterType::L:
    state.registers.l = value & BYTE_MASK;
    break;
  case RegisterType::AF:
    state.registers.a = (value >> 8) & BYTE_MASK;
    state.registers.f = value & 0xF0;
    break;
  case RegisterType::BC:
    state.registers.b = (value >> 8) & BYTE_MASK;
    state.registers.c = value & BYTE_MASK;
    break;
  case RegisterType::DE:
    state.registers.d = (value >> 8) & BYTE_MASK;
    state.registers.e = value & BYTE_MASK;
    break;
  case RegisterType::HL:
    state.registers.h = (value >> 8) & BYTE_MASK;
    state.registers.l = value & BYTE_MASK;
    break;
  case RegisterType::SP:
    state.registers.sp = value;
    break;
  case RegisterType::PC:
    state.registers.pc = value;
    break;
  case RegisterType::NONE:
    throw std::runtime_error("Trying to write to invalid CPU register");
  default:
    throw std::runtime_error("Trying to write to invalid CPU register");
  }
}

void CPU::stackPush8(uint8_t value)
{
  uint16_t sp = state.registers.sp - 1;
  setRegister16(RegisterType::SP, sp);
  bus->write8(sp, value);
}

void CPU::stackPush16(uint16_t value)
{
  stackPush8((value >> 8) & 0xFF);
  stackPush8(value & 0xFF);
}

uint8_t CPU::stackPop8()
{
  uint16_t sp = state.registers.sp;
  uint8_t value = bus->read8(sp);
  setRegister16(RegisterType::SP, sp + 1);
  return value;
}

uint16_t CPU::stackPop16()
{
  uint16_t low = stackPop8();
  uint16_t high = stackPop8();

  return (high << 8) | low;
}

void CPU::setBit(uint8_t value, uint8_t bit)
{
  if (value)
  {
    state.registers.f |= (1 << bit);
  }
  else
  {
    state.registers.f &= ~(1 << bit);
  }
}

void CPU::setFlags(int z, int n, int h, int c)
{
  if (z != -1)
  {
    setBit(z, FLAG_Z_BIT);
  }

  if (n != -1)
  {
    setBit(n, FLAG_N_BIT);
  }

  if (h != -1)
  {
    setBit(h, FLAG_H_BIT);
  }

  if (c != -1)
  {
    setBit(c, FLAG_C_BIT);
  }
}

bool CPU::is16Bit(RegisterType reg)
{
  return reg >= RegisterType::AF;
}

bool CPU::isFlagSet(uint8_t flags, uint8_t bit) const { return (flags & (1 << bit)) != 0; }
int CPU::FLAG_Z() const { return isFlagSet(state.registers.f, FLAG_Z_BIT); }
int CPU::FLAG_N() const { return isFlagSet(state.registers.f, FLAG_N_BIT); }
int CPU::FLAG_H() const { return isFlagSet(state.registers.f, FLAG_H_BIT); }
int CPU::FLAG_C() const { return isFlagSet(state.registers.f, FLAG_C_BIT); }

RegisterType CPU::decodeRegister(uint8_t value)
{
  if (value > 0b111)
  {
    return RegisterType::NONE;
  }
  return registerLookup[value];
}

bool CPU::conditionCheck() const
{
  switch (state.instruction.condition)
  {
  case ConditionType::NONE:
    return true;
  case ConditionType::C:
    return FLAG_C();
  case ConditionType::NC:
    return !FLAG_C();
  case ConditionType::Z:
    return FLAG_Z();
  case ConditionType::NZ:
    return !FLAG_Z();
  default:
    throw std::runtime_error("Unknown condition type");
  }

  return false;
}

void CPU::jumpToAddress(uint16_t addr, bool pushPC)
{
  if (!conditionCheck())
  {
    return;
  }

  if (pushPC)
  {
    cycleCallback(2);
    stackPush16(state.registers.pc);
  }

  state.registers.pc = addr;
  cycleCallback(1);
}

void CPU::requestInterrupt(InterruptType type)
{
  interrupt.requestInterrupt(type);
}

void CPU::setInterruptEnable(uint8_t value)
{
  interrupt.setInterruptEnable(value);
}

uint8_t CPU::getInterruptEnable() const
{
  return interrupt.getInterruptEnable();
}

void CPU::setInterruptFlags(uint8_t value)
{
  state.intf = value;
}

uint8_t CPU::getInterruptFlags() const
{
  return state.intf;
}
