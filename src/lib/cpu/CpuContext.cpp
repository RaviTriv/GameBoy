#include "CpuContext.h"
#include "Bus.h"
#include "Cpu.h"

static RegisterType registerLookupTable[] = {
    RegisterType::B, RegisterType::C, RegisterType::D,  RegisterType::E,
    RegisterType::H, RegisterType::L, RegisterType::HL, RegisterType::A};

CpuContext::CpuContext(CpuState state, CycleCallbackFn cycleCallback, void *cycleCallbackCtx, Bus *&bus)
    : state(state), cycleCallback(cycleCallback), cycleCallbackCtx(cycleCallbackCtx), bus(bus) {}

uint8_t CpuContext::readRegister8(RegisterType reg) const {
  switch (reg) {
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

uint16_t CpuContext::readRegister16(RegisterType reg) const {
  switch (reg) {
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

void CpuContext::setRegister8(RegisterType reg, uint8_t value) {
  switch (reg) {
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

void CpuContext::setRegister16(RegisterType reg, uint16_t value) {
  switch (reg) {
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

void CpuContext::stackPush8(uint8_t value) {
  uint16_t sp = state.registers.sp - 1;
  setRegister16(RegisterType::SP, sp);
  bus->write8(sp, value);
}

void CpuContext::stackPush16(uint16_t value) {
  stackPush8((value >> 8) & 0xFF);
  stackPush8(value & 0xFF);
}

uint8_t CpuContext::stackPop8() {
  uint16_t sp = state.registers.sp;
  uint8_t value = bus->read8(sp);
  setRegister16(RegisterType::SP, sp + 1);
  return value;
}

uint16_t CpuContext::stackPop16() {
  uint16_t low = stackPop8();
  uint16_t high = stackPop8();
  return (high << 8) | low;
}

void CpuContext::setBit(uint8_t value, uint8_t bit) {
  if (value) {
    state.registers.f |= (1 << bit);
  } else {
    state.registers.f &= ~(1 << bit);
  }
}

void CpuContext::setFlags(int z, int n, int h, int c) {
  if (z != -1) {
    setBit(z, CTX_FLAG_Z_BIT);
  }
  if (n != -1) {
    setBit(n, CTX_FLAG_N_BIT);
  }
  if (h != -1) {
    setBit(h, CTX_FLAG_H_BIT);
  }
  if (c != -1) {
    setBit(c, CTX_FLAG_C_BIT);
  }
}

bool CpuContext::is16Bit(RegisterType reg) const {
  return reg >= RegisterType::AF;
}

bool CpuContext::isFlagSet(uint8_t flags, uint8_t bit) const {
  return (flags & (1 << bit)) != 0;
}

int CpuContext::FLAG_Z() const {
  return isFlagSet(state.registers.f, CTX_FLAG_Z_BIT);
}

int CpuContext::FLAG_N() const {
  return isFlagSet(state.registers.f, CTX_FLAG_N_BIT);
}

int CpuContext::FLAG_H() const {
  return isFlagSet(state.registers.f, CTX_FLAG_H_BIT);
}

int CpuContext::FLAG_C() const {
  return isFlagSet(state.registers.f, CTX_FLAG_C_BIT);
}

RegisterType CpuContext::decodeRegister(uint8_t value) {
  if (value > 0b111) {
    return RegisterType::NONE;
  }
  return registerLookupTable[value];
}

bool CpuContext::conditionCheck() const {
  switch (state.instruction.condition) {
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

void CpuContext::jumpToAddress(uint16_t addr, bool pushPC) {
  if (!conditionCheck()) {
    return;
  }
  if (pushPC) {
    cycle(2);
    stackPush16(state.registers.pc);
  }
  state.registers.pc = addr;
  cycle(1);
}
