#include "Cpu.h"

#include <array>
#include <bit>
#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>

#include "Common.h"
#include "IMemAccess.h"
#include "Interrupts.h"
#include "Serializable.h"

namespace {

constexpr uint16_t toWord(int high, int low) {
  return static_cast<uint16_t>((high << 8) | low);
}

constexpr uint8_t HL_INDIRECT = 6;

constexpr std::array<uint8_t Registers::*, 8> R8_REGISTERS = {
    &Registers::b, &Registers::c, &Registers::d, &Registers::e,
    &Registers::h, &Registers::l, nullptr,       &Registers::a};

[[noreturn]] void unknownInstruction() {
  throw std::runtime_error("Unknown Instruction Type");
}

}  // namespace

CPU::CPU(Interrupts& interrupts, IMemAccess& memory,
         std::function_ref<void(int)> cycleCallback)
    : interrupts_(interrupts), memory_(memory), cycleCallback_(cycleCallback) {
  state.registers.a = 0x01;
  state.registers.f = 0xB0;
  state.registers.b = 0x00;
  state.registers.c = 0x13;
  state.registers.d = 0x00;
  state.registers.e = 0xD8;
  state.registers.h = 0x01;
  state.registers.l = 0x4D;
  state.registers.sp = 0xFFFE;
  state.registers.pc = 0x0100;
  state.imeScheduled = false;
  state.ime = false;
}

inline void CPU::stackPush8(uint8_t val) {
  state.registers.sp--;
  memory_.write8(state.registers.sp, val);
}

inline uint8_t CPU::stackPop8(bool idu) {
  uint8_t val = idu ? memory_.read8WithIdu(state.registers.sp)
                    : memory_.read8(state.registers.sp);
  state.registers.sp++;
  return val;
}

inline uint8_t CPU::fetch8() {
  uint8_t value = memory_.read8WithIdu(state.registers.pc++);
  cycle(1);
  return value;
}

inline uint16_t CPU::fetch16() {
  uint8_t lo = fetch8();
  uint8_t hi = fetch8();
  return toWord(hi, lo);
}

inline void CPU::push16(uint16_t value) {
  memory_.idu(state.registers.sp);
  cycle(1);
  stackPush8(highByte(value));
  cycle(1);
  stackPush8(lowByte(value));
}

inline uint16_t CPU::pop16() {
  uint8_t lo = stackPop8(true);
  cycle(1);
  uint8_t hi = stackPop8(false);
  cycle(1);
  return toWord(hi, lo);
}

inline void CPU::call(uint16_t address) {
  push16(state.registers.pc);
  state.registers.pc = address;
  cycle(1);
}

inline void CPU::ret() {
  state.registers.pc = pop16();
  cycle(1);
}

inline void CPU::halt() {
  if (interrupts_.anyPending()) {
    state.haltBug = true;
  } else {
    state.halted = true;
  }
}

void CPU::handleInterrupts() {
  if (!interrupts_.anyPending()) {
    return;
  }
  state.halted = false;
  state.ime = false;
  if (state.haltBug) {
    state.registers.pc--;
    state.haltBug = false;
  }
  cycle(2);
  memory_.idu(state.registers.sp);
  cycle(1);
  uint16_t pc = state.registers.pc;
  stackPush8(highByte(pc));
  uint8_t pending = interrupts_.ie();
  cycle(1);
  pending = static_cast<uint8_t>(pending & interrupts_.intf() & 0x1F);
  stackPush8(lowByte(pc));
  if (pending != 0) {
    auto bit = std::countr_zero(pending);
    interrupts_.setIntf(
        static_cast<uint8_t>(interrupts_.intf() & ~(1U << bit)));
    state.registers.pc = static_cast<uint16_t>(0x40 + (bit * 8));
  } else {
    state.registers.pc = 0;
  }
  cycle(1);
}

void CPU::save(std::ostream& out) const { serial::saveFields(out, *this); }

void CPU::load(std::istream& in) { serial::loadFields(in, *this); }

template <uint8_t Idx>
inline uint8_t CPU::readR8() {
  if constexpr (Idx == HL_INDIRECT) {
    uint8_t value = memory_.read8(state.registers.hl());
    cycle(1);
    return value;
  } else {
    constexpr auto reg = R8_REGISTERS[Idx];
    return state.registers.*reg;
  }
}

template <uint8_t Idx>
inline void CPU::writeR8(uint8_t value) {
  if constexpr (Idx == HL_INDIRECT) {
    memory_.write8(state.registers.hl(), value);
    cycle(1);
  } else {
    constexpr auto reg = R8_REGISTERS[Idx];
    state.registers.*reg = value;
  }
}

template <uint8_t P>
inline uint16_t CPU::rp() const {
  if constexpr (P == 0) {
    return state.registers.bc();
  } else if constexpr (P == 1) {
    return state.registers.de();
  } else if constexpr (P == 2) {
    return state.registers.hl();
  } else {
    return state.registers.sp;
  }
}

template <uint8_t P>
inline void CPU::setRp(uint16_t value) {
  if constexpr (P == 0) {
    state.registers.setBc(value);
  } else if constexpr (P == 1) {
    state.registers.setDe(value);
  } else if constexpr (P == 2) {
    state.registers.setHl(value);
  } else {
    state.registers.sp = value;
  }
}

template <uint8_t P>
inline uint16_t CPU::rp2() const {
  if constexpr (P == 3) {
    return state.registers.af();
  } else {
    return rp<P>();
  }
}

template <uint8_t P>
inline void CPU::setRp2(uint16_t value) {
  if constexpr (P == 3) {
    state.registers.setAf(value);
  } else {
    setRp<P>(value);
  }
}

template <uint8_t Cc>
inline bool CPU::cond() const {
  if constexpr (Cc == 0) {
    return !flagZ();
  } else if constexpr (Cc == 1) {
    return flagZ();
  } else if constexpr (Cc == 2) {
    return !flagC();
  } else {
    return flagC();
  }
}

template <uint8_t Op>
inline void CPU::alu(uint8_t value) {
  const int a = state.registers.a;
  const int v = value;
  if constexpr (Op == 0 || Op == 1) {
    const int c = Op == 1 ? flagC() : 0;
    state.registers.a = toU8(a + v + c);
    setFlags(state.registers.a == 0, 0,
             (a & NIBBLE_MASK) + (v & NIBBLE_MASK) + c > NIBBLE_MASK,
             a + v + c > BYTE_MASK);
  } else if constexpr (Op == 2 || Op == 3 || Op == 7) {
    const int c = Op == 3 ? flagC() : 0;
    const int result = a - v - c;
    if constexpr (Op != 7) {
      state.registers.a = toU8(result);
    }
    setFlags(toU8(result) == 0, 1,
             (a & NIBBLE_MASK) - (v & NIBBLE_MASK) - c < 0, result < 0);
  } else if constexpr (Op == 4) {
    state.registers.a &= value;
    setFlags(state.registers.a == 0, 0, 1, 0);
  } else if constexpr (Op == 5) {
    state.registers.a ^= value;
    setFlags(state.registers.a == 0, 0, 0, 0);
  } else {
    state.registers.a |= value;
    setFlags(state.registers.a == 0, 0, 0, 0);
  }
}

template <uint8_t Op>
inline uint8_t CPU::shift(uint8_t value) {
  uint8_t result = 0;
  int carry = 0;
  if constexpr (Op == 0) {
    carry = (value & BIT7_MASK) != 0;
    result = toU8((value << 1) | carry);
  } else if constexpr (Op == 1) {
    carry = value & BIT0_MASK;
    result = toU8((value >> 1) | (value << 7));
  } else if constexpr (Op == 2) {
    carry = (value & BIT7_MASK) != 0;
    result = toU8((value << 1) | flagC());
  } else if constexpr (Op == 3) {
    carry = value & BIT0_MASK;
    result = toU8((value >> 1) | (flagC() << 7));
  } else if constexpr (Op == 4) {
    carry = (value & BIT7_MASK) != 0;
    result = toU8(value << 1);
  } else if constexpr (Op == 5) {
    carry = value & BIT0_MASK;
    result = static_cast<uint8_t>(static_cast<int8_t>(value) >> 1);
  } else if constexpr (Op == 6) {
    result = toU8((value >> 4) | (value << 4));
  } else {
    carry = value & BIT0_MASK;
    result = toU8(value >> 1);
  }
  setFlags(result == 0, 0, 0, carry);
  return result;
}

template <uint8_t Op>
inline void CPU::execute() {
  constexpr uint8_t x = Op >> 6;
  constexpr uint8_t y = (Op >> 3) & 7;
  constexpr uint8_t z = Op & 7;
  constexpr uint8_t p = y >> 1;
  constexpr uint8_t q = y & 1;
  auto& r = state.registers;

  if constexpr (Op == 0x76) { /* HALT */
    halt();
  } else if constexpr (x == 1) { /* LD r,r */
    writeR8<y>(readR8<z>());
  } else if constexpr (x == 2) { /* ALU A,r */
    alu<y>(readR8<z>());
  } else if constexpr (x == 0 && z == 0) {
    if constexpr (y == 0) {        /* NOP */
    } else if constexpr (y == 1) { /* LD (a16),SP */
      uint16_t address = fetch16();
      memory_.write8(address, lowByte(r.sp));
      cycle(1);
      memory_.write8(static_cast<uint16_t>(address + 1), highByte(r.sp));
      cycle(1);
    } else if constexpr (y == 2) { /* STOP */
      (void)memory_.read8(r.pc++);
      memory_.write8(TIMER_DIV_REGISTER, 0);
    } else { /* JR r8, JR cc,r8 */
      auto offset = static_cast<int8_t>(fetch8());
      if (y == 3 || cond<y & 3>()) {
        memory_.idu(r.pc);
        r.pc = static_cast<uint16_t>(r.pc + offset);
        cycle(1);
      }
    }
  } else if constexpr (x == 0 && z == 1) {
    if constexpr (q == 0) { /* LD rr,d16 */
      setRp<p>(fetch16());
    } else { /* ADD HL,rr */
      uint32_t hl = r.hl();
      uint32_t value = rp<p>();
      uint32_t result = hl + value;
      cycle(1);
      int h = (hl & HALF_CARRY_16_MASK) + (value & HALF_CARRY_16_MASK) >=
              HALF_CARRY_16_THRESHOLD;
      int c = result >= CARRY_16_THRESHOLD;
      r.setHl(result & WORD_MASK);
      setFlags(-1, 0, h, c);
    }
  } else if constexpr (x == 0 && z == 2) { /* LD (rr),A / LD A,(rr) */
    uint16_t address = rp<p == 3 ? 2 : p>();
    if constexpr (q == 0) {
      memory_.write8(address, r.a);
    } else if constexpr (p >= 2) {
      r.a = memory_.read8WithIdu(address);
    } else {
      r.a = memory_.read8(address);
    }
    if constexpr (p == 2) {
      r.setHl(address + 1);
    } else if constexpr (p == 3) {
      r.setHl(address - 1);
    }
    cycle(1);
  } else if constexpr (x == 0 && z == 3) { /* INC rr / DEC rr */
    memory_.idu(rp<p>());
    setRp<p>(static_cast<uint16_t>(rp<p>() + (q == 0 ? 1 : -1)));
    cycle(1);
  } else if constexpr (x == 0 && z == 4) { /* INC r */
    uint8_t value = toU8(readR8<y>() + 1);
    writeR8<y>(value);
    setFlags(value == 0, 0, (value & NIBBLE_MASK) == 0, -1);
  } else if constexpr (x == 0 && z == 5) { /* DEC r */
    uint8_t value = toU8(readR8<y>() - 1);
    writeR8<y>(value);
    setFlags(value == 0, 1, (value & NIBBLE_MASK) == NIBBLE_MASK, -1);
  } else if constexpr (x == 0 && z == 6) { /* LD r,d8 */
    writeR8<y>(fetch8());
  } else if constexpr (x == 0 && z == 7) {
    if constexpr (y < 4) { /* RLCA, RRCA, RLA, RRA */
      r.a = shift<y>(r.a);
      setFlags(0, -1, -1, -1);
    } else if constexpr (y == 4) { /* DAA */
      uint8_t u = 0;
      int c = 0;
      if (flagH() || (!flagN() && (r.a & NIBBLE_MASK) > 9)) {
        u = 6;
      }
      if (flagC() || (!flagN() && r.a > 0x99)) {
        u |= 0x60;
        c = 1;
      }
      r.a = toU8(r.a + (flagN() ? -u : u));
      setFlags(r.a == 0, -1, 0, c);
    } else if constexpr (y == 5) { /* CPL */
      r.a = static_cast<uint8_t>(~r.a);
      setFlags(-1, 1, 1, -1);
    } else if constexpr (y == 6) { /* SCF */
      setFlags(-1, 0, 0, 1);
    } else { /* CCF */
      setFlags(-1, 0, 0, flagC() ^ 1);
    }
  } else if constexpr (z == 0) {
    if constexpr (y < 4) { /* RET cc */
      cycle(1);
      if (cond<y>()) {
        ret();
      }
    } else if constexpr (y == 4) { /* LDH (a8),A */
      uint8_t offset = fetch8();
      memory_.write8(IO_REGISTERS_START | offset, r.a);
      cycle(1);
    } else if constexpr (y == 5) { /* ADD SP,r8 */
      auto val = static_cast<int8_t>(fetch8());
      int h = (r.sp & NIBBLE_MASK) + (val & NIBBLE_MASK) >= 0x10;
      int c = (r.sp & BYTE_MASK) + (val & BYTE_MASK) >= 0x100;
      r.sp = static_cast<uint16_t>(static_cast<int>(r.sp) + val);
      cycle(1);
      cycle(1);
      setFlags(0, 0, h, c);
    } else if constexpr (y == 6) { /* LDH A,(a8) */
      uint8_t offset = fetch8();
      r.a = memory_.read8(IO_REGISTERS_START | offset);
      cycle(1);
    } else { /* LD HL,SP+r8 */
      auto val = static_cast<int8_t>(fetch8());
      uint8_t h = (r.sp & NIBBLE_MASK) + (val & NIBBLE_MASK) >= 0x10;
      uint8_t c = (r.sp & BYTE_MASK) + (val & BYTE_MASK) >= 0x100;
      setFlags(0, 0, h, c);
      r.setHl(static_cast<uint16_t>(static_cast<int>(r.sp) + val));
      cycle(1);
    }
  } else if constexpr (z == 1) {
    if constexpr (q == 0) { /* POP rr */
      setRp2<p>(pop16());
    } else if constexpr (p == 0) { /* RET */
      ret();
    } else if constexpr (p == 1) { /* RETI */
      state.ime = true;
      ret();
    } else if constexpr (p == 2) { /* JP HL */
      r.pc = r.hl();
    } else { /* LD SP,HL */
      memory_.idu(r.hl());
      r.sp = r.hl();
      cycle(1);
    }
  } else if constexpr (z == 2) {
    if constexpr (y < 4) { /* JP cc,a16 */
      uint16_t address = fetch16();
      if (cond<y>()) {
        r.pc = address;
        cycle(1);
      }
    } else if constexpr (y == 4) { /* LD (C),A */
      memory_.write8(IO_REGISTERS_START | r.c, r.a);
      cycle(1);
    } else if constexpr (y == 5) { /* LD (a16),A */
      memory_.write8(fetch16(), r.a);
      cycle(1);
    } else if constexpr (y == 6) { /* LD A,(C) */
      r.a = memory_.read8(IO_REGISTERS_START | r.c);
      cycle(1);
    } else { /* LD A,(a16) */
      r.a = memory_.read8(fetch16());
      cycle(1);
    }
  } else if constexpr (z == 3) {
    if constexpr (y == 0) { /* JP a16 */
      r.pc = fetch16();
      cycle(1);
    } else if constexpr (y == 1) { /* CB prefix */
      dispatchCB();
    } else if constexpr (y == 6) { /* DI */
      state.ime = false;
      state.imeScheduled = false;
    } else if constexpr (y == 7) { /* EI */
      state.imeScheduled = true;
    } else {
      unknownInstruction();
    }
  } else if constexpr (z == 4) {
    if constexpr (y < 4) { /* CALL cc,a16 */
      uint16_t address = fetch16();
      if (cond<y>()) {
        call(address);
      }
    } else {
      unknownInstruction();
    }
  } else if constexpr (z == 5) {
    if constexpr (q == 0) { /* PUSH rr */
      push16(rp2<p>());
      cycle(1);
    } else if constexpr (p == 0) { /* CALL a16 */
      call(fetch16());
    } else {
      unknownInstruction();
    }
  } else if constexpr (z == 6) { /* ALU A,d8 */
    alu<y>(fetch8());
  } else { /* RST */
    call(y * 8);
  }
}

template <uint8_t Op>
inline void CPU::executeCB() {
  constexpr uint8_t x = Op >> 6;
  constexpr uint8_t y = (Op >> 3) & 7;
  constexpr uint8_t z = Op & 7;

  uint8_t value = readR8<z>();
  if constexpr (x == 0) { /* RLC..SRL */
    writeR8<z>(shift<y>(value));
  } else if constexpr (x == 1) { /* BIT */
    setFlags(!(value & (1 << y)), 0, 1, -1);
  } else if constexpr (x == 2) { /* RES */
    writeR8<z>(toU8(value & ~(1 << y)));
  } else { /* SET */
    writeR8<z>(toU8(value | (1 << y)));
  }
}

void CPU::dispatchCB() {
  uint8_t sub = fetch8();
  // clang-format off
  switch (sub) {
    case 0x00: executeCB<0x00>(); break;
    case 0x01: executeCB<0x01>(); break;
    case 0x02: executeCB<0x02>(); break;
    case 0x03: executeCB<0x03>(); break;
    case 0x04: executeCB<0x04>(); break;
    case 0x05: executeCB<0x05>(); break;
    case 0x06: executeCB<0x06>(); break;
    case 0x07: executeCB<0x07>(); break;
    case 0x08: executeCB<0x08>(); break;
    case 0x09: executeCB<0x09>(); break;
    case 0x0A: executeCB<0x0A>(); break;
    case 0x0B: executeCB<0x0B>(); break;
    case 0x0C: executeCB<0x0C>(); break;
    case 0x0D: executeCB<0x0D>(); break;
    case 0x0E: executeCB<0x0E>(); break;
    case 0x0F: executeCB<0x0F>(); break;
    case 0x10: executeCB<0x10>(); break;
    case 0x11: executeCB<0x11>(); break;
    case 0x12: executeCB<0x12>(); break;
    case 0x13: executeCB<0x13>(); break;
    case 0x14: executeCB<0x14>(); break;
    case 0x15: executeCB<0x15>(); break;
    case 0x16: executeCB<0x16>(); break;
    case 0x17: executeCB<0x17>(); break;
    case 0x18: executeCB<0x18>(); break;
    case 0x19: executeCB<0x19>(); break;
    case 0x1A: executeCB<0x1A>(); break;
    case 0x1B: executeCB<0x1B>(); break;
    case 0x1C: executeCB<0x1C>(); break;
    case 0x1D: executeCB<0x1D>(); break;
    case 0x1E: executeCB<0x1E>(); break;
    case 0x1F: executeCB<0x1F>(); break;
    case 0x20: executeCB<0x20>(); break;
    case 0x21: executeCB<0x21>(); break;
    case 0x22: executeCB<0x22>(); break;
    case 0x23: executeCB<0x23>(); break;
    case 0x24: executeCB<0x24>(); break;
    case 0x25: executeCB<0x25>(); break;
    case 0x26: executeCB<0x26>(); break;
    case 0x27: executeCB<0x27>(); break;
    case 0x28: executeCB<0x28>(); break;
    case 0x29: executeCB<0x29>(); break;
    case 0x2A: executeCB<0x2A>(); break;
    case 0x2B: executeCB<0x2B>(); break;
    case 0x2C: executeCB<0x2C>(); break;
    case 0x2D: executeCB<0x2D>(); break;
    case 0x2E: executeCB<0x2E>(); break;
    case 0x2F: executeCB<0x2F>(); break;
    case 0x30: executeCB<0x30>(); break;
    case 0x31: executeCB<0x31>(); break;
    case 0x32: executeCB<0x32>(); break;
    case 0x33: executeCB<0x33>(); break;
    case 0x34: executeCB<0x34>(); break;
    case 0x35: executeCB<0x35>(); break;
    case 0x36: executeCB<0x36>(); break;
    case 0x37: executeCB<0x37>(); break;
    case 0x38: executeCB<0x38>(); break;
    case 0x39: executeCB<0x39>(); break;
    case 0x3A: executeCB<0x3A>(); break;
    case 0x3B: executeCB<0x3B>(); break;
    case 0x3C: executeCB<0x3C>(); break;
    case 0x3D: executeCB<0x3D>(); break;
    case 0x3E: executeCB<0x3E>(); break;
    case 0x3F: executeCB<0x3F>(); break;
    case 0x40: executeCB<0x40>(); break;
    case 0x41: executeCB<0x41>(); break;
    case 0x42: executeCB<0x42>(); break;
    case 0x43: executeCB<0x43>(); break;
    case 0x44: executeCB<0x44>(); break;
    case 0x45: executeCB<0x45>(); break;
    case 0x46: executeCB<0x46>(); break;
    case 0x47: executeCB<0x47>(); break;
    case 0x48: executeCB<0x48>(); break;
    case 0x49: executeCB<0x49>(); break;
    case 0x4A: executeCB<0x4A>(); break;
    case 0x4B: executeCB<0x4B>(); break;
    case 0x4C: executeCB<0x4C>(); break;
    case 0x4D: executeCB<0x4D>(); break;
    case 0x4E: executeCB<0x4E>(); break;
    case 0x4F: executeCB<0x4F>(); break;
    case 0x50: executeCB<0x50>(); break;
    case 0x51: executeCB<0x51>(); break;
    case 0x52: executeCB<0x52>(); break;
    case 0x53: executeCB<0x53>(); break;
    case 0x54: executeCB<0x54>(); break;
    case 0x55: executeCB<0x55>(); break;
    case 0x56: executeCB<0x56>(); break;
    case 0x57: executeCB<0x57>(); break;
    case 0x58: executeCB<0x58>(); break;
    case 0x59: executeCB<0x59>(); break;
    case 0x5A: executeCB<0x5A>(); break;
    case 0x5B: executeCB<0x5B>(); break;
    case 0x5C: executeCB<0x5C>(); break;
    case 0x5D: executeCB<0x5D>(); break;
    case 0x5E: executeCB<0x5E>(); break;
    case 0x5F: executeCB<0x5F>(); break;
    case 0x60: executeCB<0x60>(); break;
    case 0x61: executeCB<0x61>(); break;
    case 0x62: executeCB<0x62>(); break;
    case 0x63: executeCB<0x63>(); break;
    case 0x64: executeCB<0x64>(); break;
    case 0x65: executeCB<0x65>(); break;
    case 0x66: executeCB<0x66>(); break;
    case 0x67: executeCB<0x67>(); break;
    case 0x68: executeCB<0x68>(); break;
    case 0x69: executeCB<0x69>(); break;
    case 0x6A: executeCB<0x6A>(); break;
    case 0x6B: executeCB<0x6B>(); break;
    case 0x6C: executeCB<0x6C>(); break;
    case 0x6D: executeCB<0x6D>(); break;
    case 0x6E: executeCB<0x6E>(); break;
    case 0x6F: executeCB<0x6F>(); break;
    case 0x70: executeCB<0x70>(); break;
    case 0x71: executeCB<0x71>(); break;
    case 0x72: executeCB<0x72>(); break;
    case 0x73: executeCB<0x73>(); break;
    case 0x74: executeCB<0x74>(); break;
    case 0x75: executeCB<0x75>(); break;
    case 0x76: executeCB<0x76>(); break;
    case 0x77: executeCB<0x77>(); break;
    case 0x78: executeCB<0x78>(); break;
    case 0x79: executeCB<0x79>(); break;
    case 0x7A: executeCB<0x7A>(); break;
    case 0x7B: executeCB<0x7B>(); break;
    case 0x7C: executeCB<0x7C>(); break;
    case 0x7D: executeCB<0x7D>(); break;
    case 0x7E: executeCB<0x7E>(); break;
    case 0x7F: executeCB<0x7F>(); break;
    case 0x80: executeCB<0x80>(); break;
    case 0x81: executeCB<0x81>(); break;
    case 0x82: executeCB<0x82>(); break;
    case 0x83: executeCB<0x83>(); break;
    case 0x84: executeCB<0x84>(); break;
    case 0x85: executeCB<0x85>(); break;
    case 0x86: executeCB<0x86>(); break;
    case 0x87: executeCB<0x87>(); break;
    case 0x88: executeCB<0x88>(); break;
    case 0x89: executeCB<0x89>(); break;
    case 0x8A: executeCB<0x8A>(); break;
    case 0x8B: executeCB<0x8B>(); break;
    case 0x8C: executeCB<0x8C>(); break;
    case 0x8D: executeCB<0x8D>(); break;
    case 0x8E: executeCB<0x8E>(); break;
    case 0x8F: executeCB<0x8F>(); break;
    case 0x90: executeCB<0x90>(); break;
    case 0x91: executeCB<0x91>(); break;
    case 0x92: executeCB<0x92>(); break;
    case 0x93: executeCB<0x93>(); break;
    case 0x94: executeCB<0x94>(); break;
    case 0x95: executeCB<0x95>(); break;
    case 0x96: executeCB<0x96>(); break;
    case 0x97: executeCB<0x97>(); break;
    case 0x98: executeCB<0x98>(); break;
    case 0x99: executeCB<0x99>(); break;
    case 0x9A: executeCB<0x9A>(); break;
    case 0x9B: executeCB<0x9B>(); break;
    case 0x9C: executeCB<0x9C>(); break;
    case 0x9D: executeCB<0x9D>(); break;
    case 0x9E: executeCB<0x9E>(); break;
    case 0x9F: executeCB<0x9F>(); break;
    case 0xA0: executeCB<0xA0>(); break;
    case 0xA1: executeCB<0xA1>(); break;
    case 0xA2: executeCB<0xA2>(); break;
    case 0xA3: executeCB<0xA3>(); break;
    case 0xA4: executeCB<0xA4>(); break;
    case 0xA5: executeCB<0xA5>(); break;
    case 0xA6: executeCB<0xA6>(); break;
    case 0xA7: executeCB<0xA7>(); break;
    case 0xA8: executeCB<0xA8>(); break;
    case 0xA9: executeCB<0xA9>(); break;
    case 0xAA: executeCB<0xAA>(); break;
    case 0xAB: executeCB<0xAB>(); break;
    case 0xAC: executeCB<0xAC>(); break;
    case 0xAD: executeCB<0xAD>(); break;
    case 0xAE: executeCB<0xAE>(); break;
    case 0xAF: executeCB<0xAF>(); break;
    case 0xB0: executeCB<0xB0>(); break;
    case 0xB1: executeCB<0xB1>(); break;
    case 0xB2: executeCB<0xB2>(); break;
    case 0xB3: executeCB<0xB3>(); break;
    case 0xB4: executeCB<0xB4>(); break;
    case 0xB5: executeCB<0xB5>(); break;
    case 0xB6: executeCB<0xB6>(); break;
    case 0xB7: executeCB<0xB7>(); break;
    case 0xB8: executeCB<0xB8>(); break;
    case 0xB9: executeCB<0xB9>(); break;
    case 0xBA: executeCB<0xBA>(); break;
    case 0xBB: executeCB<0xBB>(); break;
    case 0xBC: executeCB<0xBC>(); break;
    case 0xBD: executeCB<0xBD>(); break;
    case 0xBE: executeCB<0xBE>(); break;
    case 0xBF: executeCB<0xBF>(); break;
    case 0xC0: executeCB<0xC0>(); break;
    case 0xC1: executeCB<0xC1>(); break;
    case 0xC2: executeCB<0xC2>(); break;
    case 0xC3: executeCB<0xC3>(); break;
    case 0xC4: executeCB<0xC4>(); break;
    case 0xC5: executeCB<0xC5>(); break;
    case 0xC6: executeCB<0xC6>(); break;
    case 0xC7: executeCB<0xC7>(); break;
    case 0xC8: executeCB<0xC8>(); break;
    case 0xC9: executeCB<0xC9>(); break;
    case 0xCA: executeCB<0xCA>(); break;
    case 0xCB: executeCB<0xCB>(); break;
    case 0xCC: executeCB<0xCC>(); break;
    case 0xCD: executeCB<0xCD>(); break;
    case 0xCE: executeCB<0xCE>(); break;
    case 0xCF: executeCB<0xCF>(); break;
    case 0xD0: executeCB<0xD0>(); break;
    case 0xD1: executeCB<0xD1>(); break;
    case 0xD2: executeCB<0xD2>(); break;
    case 0xD3: executeCB<0xD3>(); break;
    case 0xD4: executeCB<0xD4>(); break;
    case 0xD5: executeCB<0xD5>(); break;
    case 0xD6: executeCB<0xD6>(); break;
    case 0xD7: executeCB<0xD7>(); break;
    case 0xD8: executeCB<0xD8>(); break;
    case 0xD9: executeCB<0xD9>(); break;
    case 0xDA: executeCB<0xDA>(); break;
    case 0xDB: executeCB<0xDB>(); break;
    case 0xDC: executeCB<0xDC>(); break;
    case 0xDD: executeCB<0xDD>(); break;
    case 0xDE: executeCB<0xDE>(); break;
    case 0xDF: executeCB<0xDF>(); break;
    case 0xE0: executeCB<0xE0>(); break;
    case 0xE1: executeCB<0xE1>(); break;
    case 0xE2: executeCB<0xE2>(); break;
    case 0xE3: executeCB<0xE3>(); break;
    case 0xE4: executeCB<0xE4>(); break;
    case 0xE5: executeCB<0xE5>(); break;
    case 0xE6: executeCB<0xE6>(); break;
    case 0xE7: executeCB<0xE7>(); break;
    case 0xE8: executeCB<0xE8>(); break;
    case 0xE9: executeCB<0xE9>(); break;
    case 0xEA: executeCB<0xEA>(); break;
    case 0xEB: executeCB<0xEB>(); break;
    case 0xEC: executeCB<0xEC>(); break;
    case 0xED: executeCB<0xED>(); break;
    case 0xEE: executeCB<0xEE>(); break;
    case 0xEF: executeCB<0xEF>(); break;
    case 0xF0: executeCB<0xF0>(); break;
    case 0xF1: executeCB<0xF1>(); break;
    case 0xF2: executeCB<0xF2>(); break;
    case 0xF3: executeCB<0xF3>(); break;
    case 0xF4: executeCB<0xF4>(); break;
    case 0xF5: executeCB<0xF5>(); break;
    case 0xF6: executeCB<0xF6>(); break;
    case 0xF7: executeCB<0xF7>(); break;
    case 0xF8: executeCB<0xF8>(); break;
    case 0xF9: executeCB<0xF9>(); break;
    case 0xFA: executeCB<0xFA>(); break;
    case 0xFB: executeCB<0xFB>(); break;
    case 0xFC: executeCB<0xFC>(); break;
    case 0xFD: executeCB<0xFD>(); break;
    case 0xFE: executeCB<0xFE>(); break;
    case 0xFF: executeCB<0xFF>(); break;
  }
  // clang-format on
}

void CPU::step() {
  if (state.halted) [[unlikely]] {
    interrupts_.clearLate();
    cycle(1);
    if (interrupts_.anyPendingEarly()) {
      state.halted = false;
    }
  } else {
    uint8_t opcode = 0;
    if (state.haltBug) {
      opcode = memory_.read8(state.registers.pc);
      state.haltBug = false;
    } else {
      opcode = memory_.read8WithIdu(state.registers.pc++);
    }
    cycle(1);

    // clang-format off
    switch (opcode) {
      case 0x00: execute<0x00>(); break;
      case 0x01: execute<0x01>(); break;
      case 0x02: execute<0x02>(); break;
      case 0x03: execute<0x03>(); break;
      case 0x04: execute<0x04>(); break;
      case 0x05: execute<0x05>(); break;
      case 0x06: execute<0x06>(); break;
      case 0x07: execute<0x07>(); break;
      case 0x08: execute<0x08>(); break;
      case 0x09: execute<0x09>(); break;
      case 0x0A: execute<0x0A>(); break;
      case 0x0B: execute<0x0B>(); break;
      case 0x0C: execute<0x0C>(); break;
      case 0x0D: execute<0x0D>(); break;
      case 0x0E: execute<0x0E>(); break;
      case 0x0F: execute<0x0F>(); break;
      case 0x10: execute<0x10>(); break;
      case 0x11: execute<0x11>(); break;
      case 0x12: execute<0x12>(); break;
      case 0x13: execute<0x13>(); break;
      case 0x14: execute<0x14>(); break;
      case 0x15: execute<0x15>(); break;
      case 0x16: execute<0x16>(); break;
      case 0x17: execute<0x17>(); break;
      case 0x18: execute<0x18>(); break;
      case 0x19: execute<0x19>(); break;
      case 0x1A: execute<0x1A>(); break;
      case 0x1B: execute<0x1B>(); break;
      case 0x1C: execute<0x1C>(); break;
      case 0x1D: execute<0x1D>(); break;
      case 0x1E: execute<0x1E>(); break;
      case 0x1F: execute<0x1F>(); break;
      case 0x20: execute<0x20>(); break;
      case 0x21: execute<0x21>(); break;
      case 0x22: execute<0x22>(); break;
      case 0x23: execute<0x23>(); break;
      case 0x24: execute<0x24>(); break;
      case 0x25: execute<0x25>(); break;
      case 0x26: execute<0x26>(); break;
      case 0x27: execute<0x27>(); break;
      case 0x28: execute<0x28>(); break;
      case 0x29: execute<0x29>(); break;
      case 0x2A: execute<0x2A>(); break;
      case 0x2B: execute<0x2B>(); break;
      case 0x2C: execute<0x2C>(); break;
      case 0x2D: execute<0x2D>(); break;
      case 0x2E: execute<0x2E>(); break;
      case 0x2F: execute<0x2F>(); break;
      case 0x30: execute<0x30>(); break;
      case 0x31: execute<0x31>(); break;
      case 0x32: execute<0x32>(); break;
      case 0x33: execute<0x33>(); break;
      case 0x34: execute<0x34>(); break;
      case 0x35: execute<0x35>(); break;
      case 0x36: execute<0x36>(); break;
      case 0x37: execute<0x37>(); break;
      case 0x38: execute<0x38>(); break;
      case 0x39: execute<0x39>(); break;
      case 0x3A: execute<0x3A>(); break;
      case 0x3B: execute<0x3B>(); break;
      case 0x3C: execute<0x3C>(); break;
      case 0x3D: execute<0x3D>(); break;
      case 0x3E: execute<0x3E>(); break;
      case 0x3F: execute<0x3F>(); break;
      case 0x40: execute<0x40>(); break;
      case 0x41: execute<0x41>(); break;
      case 0x42: execute<0x42>(); break;
      case 0x43: execute<0x43>(); break;
      case 0x44: execute<0x44>(); break;
      case 0x45: execute<0x45>(); break;
      case 0x46: execute<0x46>(); break;
      case 0x47: execute<0x47>(); break;
      case 0x48: execute<0x48>(); break;
      case 0x49: execute<0x49>(); break;
      case 0x4A: execute<0x4A>(); break;
      case 0x4B: execute<0x4B>(); break;
      case 0x4C: execute<0x4C>(); break;
      case 0x4D: execute<0x4D>(); break;
      case 0x4E: execute<0x4E>(); break;
      case 0x4F: execute<0x4F>(); break;
      case 0x50: execute<0x50>(); break;
      case 0x51: execute<0x51>(); break;
      case 0x52: execute<0x52>(); break;
      case 0x53: execute<0x53>(); break;
      case 0x54: execute<0x54>(); break;
      case 0x55: execute<0x55>(); break;
      case 0x56: execute<0x56>(); break;
      case 0x57: execute<0x57>(); break;
      case 0x58: execute<0x58>(); break;
      case 0x59: execute<0x59>(); break;
      case 0x5A: execute<0x5A>(); break;
      case 0x5B: execute<0x5B>(); break;
      case 0x5C: execute<0x5C>(); break;
      case 0x5D: execute<0x5D>(); break;
      case 0x5E: execute<0x5E>(); break;
      case 0x5F: execute<0x5F>(); break;
      case 0x60: execute<0x60>(); break;
      case 0x61: execute<0x61>(); break;
      case 0x62: execute<0x62>(); break;
      case 0x63: execute<0x63>(); break;
      case 0x64: execute<0x64>(); break;
      case 0x65: execute<0x65>(); break;
      case 0x66: execute<0x66>(); break;
      case 0x67: execute<0x67>(); break;
      case 0x68: execute<0x68>(); break;
      case 0x69: execute<0x69>(); break;
      case 0x6A: execute<0x6A>(); break;
      case 0x6B: execute<0x6B>(); break;
      case 0x6C: execute<0x6C>(); break;
      case 0x6D: execute<0x6D>(); break;
      case 0x6E: execute<0x6E>(); break;
      case 0x6F: execute<0x6F>(); break;
      case 0x70: execute<0x70>(); break;
      case 0x71: execute<0x71>(); break;
      case 0x72: execute<0x72>(); break;
      case 0x73: execute<0x73>(); break;
      case 0x74: execute<0x74>(); break;
      case 0x75: execute<0x75>(); break;
      case 0x76: execute<0x76>(); break;
      case 0x77: execute<0x77>(); break;
      case 0x78: execute<0x78>(); break;
      case 0x79: execute<0x79>(); break;
      case 0x7A: execute<0x7A>(); break;
      case 0x7B: execute<0x7B>(); break;
      case 0x7C: execute<0x7C>(); break;
      case 0x7D: execute<0x7D>(); break;
      case 0x7E: execute<0x7E>(); break;
      case 0x7F: execute<0x7F>(); break;
      case 0x80: execute<0x80>(); break;
      case 0x81: execute<0x81>(); break;
      case 0x82: execute<0x82>(); break;
      case 0x83: execute<0x83>(); break;
      case 0x84: execute<0x84>(); break;
      case 0x85: execute<0x85>(); break;
      case 0x86: execute<0x86>(); break;
      case 0x87: execute<0x87>(); break;
      case 0x88: execute<0x88>(); break;
      case 0x89: execute<0x89>(); break;
      case 0x8A: execute<0x8A>(); break;
      case 0x8B: execute<0x8B>(); break;
      case 0x8C: execute<0x8C>(); break;
      case 0x8D: execute<0x8D>(); break;
      case 0x8E: execute<0x8E>(); break;
      case 0x8F: execute<0x8F>(); break;
      case 0x90: execute<0x90>(); break;
      case 0x91: execute<0x91>(); break;
      case 0x92: execute<0x92>(); break;
      case 0x93: execute<0x93>(); break;
      case 0x94: execute<0x94>(); break;
      case 0x95: execute<0x95>(); break;
      case 0x96: execute<0x96>(); break;
      case 0x97: execute<0x97>(); break;
      case 0x98: execute<0x98>(); break;
      case 0x99: execute<0x99>(); break;
      case 0x9A: execute<0x9A>(); break;
      case 0x9B: execute<0x9B>(); break;
      case 0x9C: execute<0x9C>(); break;
      case 0x9D: execute<0x9D>(); break;
      case 0x9E: execute<0x9E>(); break;
      case 0x9F: execute<0x9F>(); break;
      case 0xA0: execute<0xA0>(); break;
      case 0xA1: execute<0xA1>(); break;
      case 0xA2: execute<0xA2>(); break;
      case 0xA3: execute<0xA3>(); break;
      case 0xA4: execute<0xA4>(); break;
      case 0xA5: execute<0xA5>(); break;
      case 0xA6: execute<0xA6>(); break;
      case 0xA7: execute<0xA7>(); break;
      case 0xA8: execute<0xA8>(); break;
      case 0xA9: execute<0xA9>(); break;
      case 0xAA: execute<0xAA>(); break;
      case 0xAB: execute<0xAB>(); break;
      case 0xAC: execute<0xAC>(); break;
      case 0xAD: execute<0xAD>(); break;
      case 0xAE: execute<0xAE>(); break;
      case 0xAF: execute<0xAF>(); break;
      case 0xB0: execute<0xB0>(); break;
      case 0xB1: execute<0xB1>(); break;
      case 0xB2: execute<0xB2>(); break;
      case 0xB3: execute<0xB3>(); break;
      case 0xB4: execute<0xB4>(); break;
      case 0xB5: execute<0xB5>(); break;
      case 0xB6: execute<0xB6>(); break;
      case 0xB7: execute<0xB7>(); break;
      case 0xB8: execute<0xB8>(); break;
      case 0xB9: execute<0xB9>(); break;
      case 0xBA: execute<0xBA>(); break;
      case 0xBB: execute<0xBB>(); break;
      case 0xBC: execute<0xBC>(); break;
      case 0xBD: execute<0xBD>(); break;
      case 0xBE: execute<0xBE>(); break;
      case 0xBF: execute<0xBF>(); break;
      case 0xC0: execute<0xC0>(); break;
      case 0xC1: execute<0xC1>(); break;
      case 0xC2: execute<0xC2>(); break;
      case 0xC3: execute<0xC3>(); break;
      case 0xC4: execute<0xC4>(); break;
      case 0xC5: execute<0xC5>(); break;
      case 0xC6: execute<0xC6>(); break;
      case 0xC7: execute<0xC7>(); break;
      case 0xC8: execute<0xC8>(); break;
      case 0xC9: execute<0xC9>(); break;
      case 0xCA: execute<0xCA>(); break;
      case 0xCB: execute<0xCB>(); break;
      case 0xCC: execute<0xCC>(); break;
      case 0xCD: execute<0xCD>(); break;
      case 0xCE: execute<0xCE>(); break;
      case 0xCF: execute<0xCF>(); break;
      case 0xD0: execute<0xD0>(); break;
      case 0xD1: execute<0xD1>(); break;
      case 0xD2: execute<0xD2>(); break;
      case 0xD3: execute<0xD3>(); break;
      case 0xD4: execute<0xD4>(); break;
      case 0xD5: execute<0xD5>(); break;
      case 0xD6: execute<0xD6>(); break;
      case 0xD7: execute<0xD7>(); break;
      case 0xD8: execute<0xD8>(); break;
      case 0xD9: execute<0xD9>(); break;
      case 0xDA: execute<0xDA>(); break;
      case 0xDB: execute<0xDB>(); break;
      case 0xDC: execute<0xDC>(); break;
      case 0xDD: execute<0xDD>(); break;
      case 0xDE: execute<0xDE>(); break;
      case 0xDF: execute<0xDF>(); break;
      case 0xE0: execute<0xE0>(); break;
      case 0xE1: execute<0xE1>(); break;
      case 0xE2: execute<0xE2>(); break;
      case 0xE3: execute<0xE3>(); break;
      case 0xE4: execute<0xE4>(); break;
      case 0xE5: execute<0xE5>(); break;
      case 0xE6: execute<0xE6>(); break;
      case 0xE7: execute<0xE7>(); break;
      case 0xE8: execute<0xE8>(); break;
      case 0xE9: execute<0xE9>(); break;
      case 0xEA: execute<0xEA>(); break;
      case 0xEB: execute<0xEB>(); break;
      case 0xEC: execute<0xEC>(); break;
      case 0xED: execute<0xED>(); break;
      case 0xEE: execute<0xEE>(); break;
      case 0xEF: execute<0xEF>(); break;
      case 0xF0: execute<0xF0>(); break;
      case 0xF1: execute<0xF1>(); break;
      case 0xF2: execute<0xF2>(); break;
      case 0xF3: execute<0xF3>(); break;
      case 0xF4: execute<0xF4>(); break;
      case 0xF5: execute<0xF5>(); break;
      case 0xF6: execute<0xF6>(); break;
      case 0xF7: execute<0xF7>(); break;
      case 0xF8: execute<0xF8>(); break;
      case 0xF9: execute<0xF9>(); break;
      case 0xFA: execute<0xFA>(); break;
      case 0xFB: execute<0xFB>(); break;
      case 0xFC: execute<0xFC>(); break;
      case 0xFD: execute<0xFD>(); break;
      case 0xFE: execute<0xFE>(); break;
      case 0xFF: execute<0xFF>(); break;
    }
    // clang-format on
  }

  if (state.ime && !state.halted) [[unlikely]] {
    handleInterrupts();
    state.imeScheduled = false;
  }

  if (state.imeScheduled) {
    state.ime = true;
  }
}
