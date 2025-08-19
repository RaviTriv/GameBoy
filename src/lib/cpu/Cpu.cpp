#include "../../../include/Cpu.h"
#include "../../../include/Bus.h"
#include "../../../include/Logger.h"

CPU::CPU(CycleCallback cycleCallback, std::shared_ptr<Bus> bus) : cycleCallback(cycleCallback), decoder(this), executer(this), bus(bus)
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
};

void CPU::fetch()
{
  state.opcode = bus->read8(state.registers.pc);
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
  fetch();
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
