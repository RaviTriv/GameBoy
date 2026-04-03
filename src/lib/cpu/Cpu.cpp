#include "Cpu.h"
#include "Bus.h"

CPU::CPU(CycleCallbackFn cycleCallback, void *cycleCallbackCtx, Bus *bus)
    : cycleCallback(cycleCallback), cycleCallbackCtx(cycleCallbackCtx),
      bus(bus) {
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
  state.ie = 0x00;
  state.intf = 0;
  state.imeScheduled = false;
  state.ime = false;
}

void CPU::setBus(Bus *b) { this->bus = b; }

inline void CPU::stackPush8(uint8_t val) {
  state.registers.sp--;
  bus->write8(state.registers.sp, val);
}

inline void CPU::stackPush16(uint16_t val) {
  stackPush8((val >> 8) & BYTE_MASK);
  stackPush8(val & BYTE_MASK);
}

inline uint8_t CPU::stackPop8() {
  uint8_t val = bus->read8(state.registers.sp);
  state.registers.sp++;
  return val;
}

inline uint16_t CPU::stackPop16() {
  uint16_t low = stackPop8();
  uint16_t high = stackPop8();
  return (high << 8) | low;
}

void CPU::interruptHandle(uint16_t address) {
  stackPush16(state.registers.pc);
  state.registers.pc = address;
}

bool CPU::checkInterrupt(uint16_t address, InterruptType type) {
  if (state.intf & static_cast<int>(type) &&
      state.ie & static_cast<int>(type)) {
    interruptHandle(address);
    state.intf &= ~static_cast<int>(type);
    state.halted = false;
    state.ime = false;
    return true;
  }
  return false;
}

void CPU::handleInterrupts() {
  if (checkInterrupt(0x40, InterruptType::VBLANK)) {
  } else if (checkInterrupt(0x48, InterruptType::LCD_STAT)) {
  } else if (checkInterrupt(0x50, InterruptType::TIMER)) {
  } else if (checkInterrupt(0x58, InterruptType::SERIAL)) {
  } else if (checkInterrupt(0x60, InterruptType::JOYPAD)) {
  }
}

void CPU::requestInterrupt(InterruptType type) { state.intf |= (uint8_t)type; }

void CPU::setInterruptEnable(uint8_t value) { state.ie = value; }
uint8_t CPU::getInterruptEnable() const { return state.ie; }
void CPU::setInterruptFlags(uint8_t value) { state.intf = value; }
uint8_t CPU::getInterruptFlags() const { return state.intf; }
CPU::State CPU::getState() const { return state; }
void CPU::setState(const State &s) { this->state = s; }
InterruptRegs CPU::getInterruptRegs() { return {state.ie, state.intf}; }

void CPU::executeCB() {
  uint8_t sub = bus->read8(state.registers.pc++);
  cycle(1);

  auto cbRead = [&](uint8_t idx) -> uint8_t {
    switch (idx) {
    case 0:
      return state.registers.b;
    case 1:
      return state.registers.c;
    case 2:
      return state.registers.d;
    case 3:
      return state.registers.e;
    case 4:
      return state.registers.h;
    case 5:
      return state.registers.l;
    case 6:
      return bus->read8(state.registers.hl());
    case 7:
      return state.registers.a;
    default:
      return 0;
    }
  };

  auto cbWrite = [&](uint8_t idx, uint8_t val) {
    switch (idx) {
    case 0:
      state.registers.b = val;
      break;
    case 1:
      state.registers.c = val;
      break;
    case 2:
      state.registers.d = val;
      break;
    case 3:
      state.registers.e = val;
      break;
    case 4:
      state.registers.h = val;
      break;
    case 5:
      state.registers.l = val;
      break;
    case 6:
      bus->write8(state.registers.hl(), val);
      break;
    case 7:
      state.registers.a = val;
      break;
    }
  };

  uint8_t reg_idx = sub & 0x07;
  uint8_t op_group = sub >> 6;
  uint8_t bit_num = (sub >> 3) & 0x07;
  bool isHL = (reg_idx == 6);

  cycle(1);
  if (isHL)
    cycle(2);

  uint8_t val = cbRead(reg_idx);

  switch (op_group) {
  case 0: {
    switch (bit_num) {
    case 0: { /* RLC */
      bool setC = (val & BIT7_MASK) != 0;
      uint8_t result = ((val << 1) & BYTE_MASK) | (setC ? 1 : 0);
      cbWrite(reg_idx, result);
      setFlags(result == 0, 0, 0, setC);
    } break;
    case 1: { /* RRC */
      uint8_t old = val;
      uint8_t result = (val >> 1) | (old << 7);
      cbWrite(reg_idx, result);
      setFlags(!result, 0, 0, old & 1);
    } break;
    case 2: { /* RL */
      uint8_t old = val;
      uint8_t result = (val << 1) | flagC();
      cbWrite(reg_idx, result);
      setFlags(!result, 0, 0, !!(old & BIT7_MASK));
    } break;
    case 3: { /* RR */
      uint8_t old = val;
      uint8_t result = (val >> 1) | (flagC() << 7);
      cbWrite(reg_idx, result);
      setFlags(!result, 0, 0, old & 1);
    } break;
    case 4: { /* SLA */
      uint8_t old = val;
      uint8_t result = val << 1;
      cbWrite(reg_idx, result);
      setFlags(!result, 0, 0, !!(old & BIT7_MASK));
    } break;
    case 5: { /* SRA */
      uint8_t result = (int8_t)val >> 1;
      cbWrite(reg_idx, result);
      setFlags(!result, 0, 0, val & 1);
    } break;
    case 6: { /* SWAP */
      uint8_t result =
          ((val & HIGH_NIBBLE_MASK) >> 4) | ((val & NIBBLE_MASK) << 4);
      cbWrite(reg_idx, result);
      setFlags(result == 0, 0, 0, 0);
    } break;
    case 7: { /* SRL */
      uint8_t result = val >> 1;
      cbWrite(reg_idx, result);
      setFlags(!result, 0, 0, val & 1);
    } break;
    }
  } break;
  case 1: { /* BIT */
    setFlags(!(val & (1 << bit_num)), 0, 1, -1);
  } break;
  case 2: { /* RES */
    cbWrite(reg_idx, val & ~(1 << bit_num));
  } break;
  case 3: { /* SET */
    cbWrite(reg_idx, val | (1 << bit_num));
  } break;
  }
}

void CPU::step() {
  if (state.halted) [[unlikely]] {
    cycle(1);
    if (state.intf) {
      state.halted = false;
    }
  } else {
    uint8_t opcode = bus->read8(state.registers.pc++);
    cycle(1);

    switch (opcode) {
    case 0x00: { /* NOP */
    } break;

    case 0x01: { /* LD BC,d16 */
      uint8_t lo = bus->read8(state.registers.pc);
      cycle(1);
      uint8_t hi = bus->read8(state.registers.pc + 1);
      cycle(1);
      state.registers.pc += 2;
      state.registers.c = lo;
      state.registers.b = hi;
    } break;

    case 0x02: { /* LD (BC),A */
      bus->write8(state.registers.bc(), state.registers.a);
      cycle(1);
    } break;

    case 0x03: { /* INC BC */
      state.registers.setBc(state.registers.bc() + 1);
      cycle(1);
    } break;

    case 0x04: { /* INC B */
      state.registers.b++;
      setFlags(state.registers.b == 0, 0,
               (state.registers.b & NIBBLE_MASK) == 0, -1);
    } break;

    case 0x05: { /* DEC B */
      state.registers.b--;
      setFlags(state.registers.b == 0, 1,
               (state.registers.b & NIBBLE_MASK) == NIBBLE_MASK, -1);
    } break;

    case 0x06: { /* LD B,d8 */
      state.registers.b = bus->read8(state.registers.pc++);
      cycle(1);
    } break;

    case 0x07: { /* RLCA */
      uint8_t u = state.registers.a;
      bool c = (u >> 7) & 1;
      u = (u << 1) | c;
      state.registers.a = u;
      setFlags(0, 0, 0, c);
    } break;

    case 0x08: { /* LD (a16),SP */
      uint16_t lo = bus->read8(state.registers.pc);
      cycle(1);
      uint16_t hi = bus->read8(state.registers.pc + 1);
      cycle(1);
      uint16_t addr = lo | (hi << 8);
      state.registers.pc += 2;
      bus->write16(addr, state.registers.sp);
      cycle(1);
    } break;

    case 0x09: { /* ADD HL,BC */
      uint32_t hl = state.registers.hl();
      uint32_t bc = state.registers.bc();
      uint32_t result = hl + bc;
      cycle(1);
      int h = (hl & HALF_CARRY_16_MASK) + (bc & HALF_CARRY_16_MASK) >=
              HALF_CARRY_16_THRESHOLD;
      int c = result >= CARRY_16_THRESHOLD;
      state.registers.setHl(result & WORD_MASK);
      setFlags(-1, 0, h, c);
    } break;

    case 0x0A: { /* LD A,(BC) */
      state.registers.a = bus->read8(state.registers.bc());
      cycle(1);
    } break;

    case 0x0B: { /* DEC BC */
      state.registers.setBc(state.registers.bc() - 1);
      cycle(1);
    } break;

    case 0x0C: { /* INC C */
      state.registers.c++;
      setFlags(state.registers.c == 0, 0,
               (state.registers.c & NIBBLE_MASK) == 0, -1);
    } break;

    case 0x0D: { /* DEC C */
      state.registers.c--;
      setFlags(state.registers.c == 0, 1,
               (state.registers.c & NIBBLE_MASK) == NIBBLE_MASK, -1);
    } break;

    case 0x0E: { /* LD C,d8 */
      state.registers.c = bus->read8(state.registers.pc++);
      cycle(1);
    } break;

    case 0x0F: { /* RRCA */
      uint8_t b = state.registers.a & 1;
      state.registers.a >>= 1;
      state.registers.a |= (b << 7);
      setFlags(0, 0, 0, b);
    } break;

    case 0x10: { /* STOP */
    } break;

    case 0x11: { /* LD DE,d16 */
      uint8_t lo = bus->read8(state.registers.pc);
      cycle(1);
      uint8_t hi = bus->read8(state.registers.pc + 1);
      cycle(1);
      state.registers.pc += 2;
      state.registers.e = lo;
      state.registers.d = hi;
    } break;

    case 0x12: { /* LD (DE),A */
      bus->write8(state.registers.de(), state.registers.a);
      cycle(1);
    } break;

    case 0x13: { /* INC DE */
      state.registers.setDe(state.registers.de() + 1);
      cycle(1);
    } break;

    case 0x14: { /* INC D */
      state.registers.d++;
      setFlags(state.registers.d == 0, 0,
               (state.registers.d & NIBBLE_MASK) == 0, -1);
    } break;

    case 0x15: { /* DEC D */
      state.registers.d--;
      setFlags(state.registers.d == 0, 1,
               (state.registers.d & NIBBLE_MASK) == NIBBLE_MASK, -1);
    } break;

    case 0x16: { /* LD D,d8 */
      state.registers.d = bus->read8(state.registers.pc++);
      cycle(1);
    } break;

    case 0x17: { /* RLA */
      uint8_t u = state.registers.a;
      uint8_t cf = flagC();
      uint8_t c = (u >> 7) & 1;
      state.registers.a = (u << 1) | cf;
      setFlags(0, 0, 0, c);
    } break;

    case 0x18: { /* JR r8 */
      int8_t rel = (int8_t)bus->read8(state.registers.pc++);
      cycle(1);
      state.registers.pc += rel;
      cycle(1);
    } break;

    case 0x19: { /* ADD HL,DE */
      uint32_t hl = state.registers.hl();
      uint32_t de = state.registers.de();
      uint32_t result = hl + de;
      cycle(1);
      int h = (hl & HALF_CARRY_16_MASK) + (de & HALF_CARRY_16_MASK) >=
              HALF_CARRY_16_THRESHOLD;
      int c = result >= CARRY_16_THRESHOLD;
      state.registers.setHl(result & WORD_MASK);
      setFlags(-1, 0, h, c);
    } break;

    case 0x1A: { /* LD A,(DE) */
      state.registers.a = bus->read8(state.registers.de());
      cycle(1);
    } break;

    case 0x1B: { /* DEC DE */
      state.registers.setDe(state.registers.de() - 1);
      cycle(1);
    } break;

    case 0x1C: { /* INC E */
      state.registers.e++;
      setFlags(state.registers.e == 0, 0,
               (state.registers.e & NIBBLE_MASK) == 0, -1);
    } break;

    case 0x1D: { /* DEC E */
      state.registers.e--;
      setFlags(state.registers.e == 0, 1,
               (state.registers.e & NIBBLE_MASK) == NIBBLE_MASK, -1);
    } break;

    case 0x1E: { /* LD E,d8 */
      state.registers.e = bus->read8(state.registers.pc++);
      cycle(1);
    } break;

    case 0x1F: { /* RRA */
      uint8_t carry = flagC();
      uint8_t new_c = state.registers.a & 1;
      state.registers.a >>= 1;
      state.registers.a |= (carry << 7);
      setFlags(0, 0, 0, new_c);
    } break;

    case 0x20: { /* JR NZ,r8 */
      int8_t rel = (int8_t)bus->read8(state.registers.pc++);
      cycle(1);
      if (!flagZ()) {
        state.registers.pc += rel;
        cycle(1);
      }
    } break;

    case 0x21: { /* LD HL,d16 */
      uint8_t lo = bus->read8(state.registers.pc);
      cycle(1);
      uint8_t hi = bus->read8(state.registers.pc + 1);
      cycle(1);
      state.registers.pc += 2;
      state.registers.l = lo;
      state.registers.h = hi;
    } break;

    case 0x22: { /* LD (HL+),A */
      uint16_t hl = state.registers.hl();
      bus->write8(hl, state.registers.a);
      state.registers.setHl(hl + 1);
      cycle(1);
    } break;

    case 0x23: { /* INC HL */
      state.registers.setHl(state.registers.hl() + 1);
      cycle(1);
    } break;

    case 0x24: { /* INC H */
      state.registers.h++;
      setFlags(state.registers.h == 0, 0,
               (state.registers.h & NIBBLE_MASK) == 0, -1);
    } break;

    case 0x25: { /* DEC H */
      state.registers.h--;
      setFlags(state.registers.h == 0, 1,
               (state.registers.h & NIBBLE_MASK) == NIBBLE_MASK, -1);
    } break;

    case 0x26: { /* LD H,d8 */
      state.registers.h = bus->read8(state.registers.pc++);
      cycle(1);
    } break;

    case 0x27: { /* DAA */
      uint8_t u = 0;
      int c = 0;
      if (flagH() || (!flagN() && (state.registers.a & NIBBLE_MASK) > 9)) {
        u = 6;
      }
      if (flagC() || (!flagN() && state.registers.a > 0x99)) {
        u |= 0x60;
        c = 1;
      }
      state.registers.a += flagN() ? -u : u;
      setFlags(state.registers.a == 0, -1, 0, c);
    } break;

    case 0x28: { /* JR Z,r8 */
      int8_t rel = (int8_t)bus->read8(state.registers.pc++);
      cycle(1);
      if (flagZ()) {
        state.registers.pc += rel;
        cycle(1);
      }
    } break;

    case 0x29: { /* ADD HL,HL */
      uint32_t hl = state.registers.hl();
      uint32_t result = hl + hl;
      cycle(1);
      int h = (hl & HALF_CARRY_16_MASK) + (hl & HALF_CARRY_16_MASK) >=
              HALF_CARRY_16_THRESHOLD;
      int c = result >= CARRY_16_THRESHOLD;
      state.registers.setHl(result & WORD_MASK);
      setFlags(-1, 0, h, c);
    } break;

    case 0x2A: { /* LD A,(HL+) */
      uint16_t hl = state.registers.hl();
      state.registers.a = bus->read8(hl);
      cycle(1);
      state.registers.setHl(hl + 1);
    } break;

    case 0x2B: { /* DEC HL */
      state.registers.setHl(state.registers.hl() - 1);
      cycle(1);
    } break;

    case 0x2C: { /* INC L */
      state.registers.l++;
      setFlags(state.registers.l == 0, 0,
               (state.registers.l & NIBBLE_MASK) == 0, -1);
    } break;

    case 0x2D: { /* DEC L */
      state.registers.l--;
      setFlags(state.registers.l == 0, 1,
               (state.registers.l & NIBBLE_MASK) == NIBBLE_MASK, -1);
    } break;

    case 0x2E: { /* LD L,d8 */
      state.registers.l = bus->read8(state.registers.pc++);
      cycle(1);
    } break;

    case 0x2F: { /* CPL */
      state.registers.a = ~state.registers.a;
      setFlags(-1, 1, 1, -1);
    } break;

    case 0x30: { /* JR NC,r8 */
      int8_t rel = (int8_t)bus->read8(state.registers.pc++);
      cycle(1);
      if (!flagC()) {
        state.registers.pc += rel;
        cycle(1);
      }
    } break;

    case 0x31: { /* LD SP,d16 */
      uint16_t lo = bus->read8(state.registers.pc);
      cycle(1);
      uint16_t hi = bus->read8(state.registers.pc + 1);
      cycle(1);
      state.registers.pc += 2;
      state.registers.sp = lo | (hi << 8);
    } break;

    case 0x32: { /* LD (HL-),A */
      uint16_t hl = state.registers.hl();
      bus->write8(hl, state.registers.a);
      state.registers.setHl(hl - 1);
      cycle(1);
    } break;

    case 0x33: { /* INC SP */
      state.registers.sp++;
      cycle(1);
    } break;

    case 0x34: { /* INC (HL) */
      uint16_t hl = state.registers.hl();
      uint8_t val = bus->read8(hl);
      cycle(1);
      val++;
      bus->write8(hl, val);
      setFlags(val == 0, 0, (val & NIBBLE_MASK) == 0, -1);
    } break;

    case 0x35: { /* DEC (HL) */
      uint16_t hl = state.registers.hl();
      uint8_t val = bus->read8(hl);
      cycle(1);
      val--;
      bus->write8(hl, val);
      setFlags(val == 0, 1, (val & NIBBLE_MASK) == NIBBLE_MASK, -1);
    } break;

    case 0x36: { /* LD (HL),d8 */
      uint8_t val = bus->read8(state.registers.pc++);
      cycle(1);
      bus->write8(state.registers.hl(), val);
      cycle(1);
    } break;

    case 0x37: { /* SCF */
      setFlags(-1, 0, 0, 1);
    } break;

    case 0x38: { /* JR C,r8 */
      int8_t rel = (int8_t)bus->read8(state.registers.pc++);
      cycle(1);
      if (flagC()) {
        state.registers.pc += rel;
        cycle(1);
      }
    } break;

    case 0x39: { /* ADD HL,SP */
      uint32_t hl = state.registers.hl();
      uint32_t sp = state.registers.sp;
      uint32_t result = hl + sp;
      cycle(1);
      int h = (hl & HALF_CARRY_16_MASK) + (sp & HALF_CARRY_16_MASK) >=
              HALF_CARRY_16_THRESHOLD;
      int c = result >= CARRY_16_THRESHOLD;
      state.registers.setHl(result & WORD_MASK);
      setFlags(-1, 0, h, c);
    } break;

    case 0x3A: { /* LD A,(HL-) */
      uint16_t hl = state.registers.hl();
      state.registers.a = bus->read8(hl);
      cycle(1);
      state.registers.setHl(hl - 1);
    } break;

    case 0x3B: { /* DEC SP */
      state.registers.sp--;
      cycle(1);
    } break;

    case 0x3C: { /* INC A */
      state.registers.a++;
      setFlags(state.registers.a == 0, 0,
               (state.registers.a & NIBBLE_MASK) == 0, -1);
    } break;

    case 0x3D: { /* DEC A */
      state.registers.a--;
      setFlags(state.registers.a == 0, 1,
               (state.registers.a & NIBBLE_MASK) == NIBBLE_MASK, -1);
    } break;

    case 0x3E: { /* LD A,d8 */
      state.registers.a = bus->read8(state.registers.pc++);
      cycle(1);
    } break;

    case 0x3F: { /* CCF */
      setFlags(-1, 0, 0, flagC() ^ 1);
    } break;

    case 0x40: /* LD B,B */
      state.registers.b = state.registers.b;
      break;
    case 0x41: /* LD B,C */
      state.registers.b = state.registers.c;
      break;
    case 0x42: /* LD B,D */
      state.registers.b = state.registers.d;
      break;
    case 0x43: /* LD B,E */
      state.registers.b = state.registers.e;
      break;
    case 0x44: /* LD B,H */
      state.registers.b = state.registers.h;
      break;
    case 0x45: /* LD B,L */
      state.registers.b = state.registers.l;
      break;
    case 0x46: /* LD B,(HL) */
      state.registers.b = bus->read8(state.registers.hl());
      cycle(1);
      break;
    case 0x47: /* LD B,A */
      state.registers.b = state.registers.a;
      break;
    case 0x48: /* LD C,B */
      state.registers.c = state.registers.b;
      break;
    case 0x49: /* LD C,C */
      state.registers.c = state.registers.c;
      break;
    case 0x4A: /* LD C,D */
      state.registers.c = state.registers.d;
      break;
    case 0x4B: /* LD C,E */
      state.registers.c = state.registers.e;
      break;
    case 0x4C: /* LD C,H */
      state.registers.c = state.registers.h;
      break;
    case 0x4D: /* LD C,L */
      state.registers.c = state.registers.l;
      break;
    case 0x4E: /* LD C,(HL) */
      state.registers.c = bus->read8(state.registers.hl());
      cycle(1);
      break;
    case 0x4F: /* LD C,A */
      state.registers.c = state.registers.a;
      break;
    case 0x50: /* LD D,B */
      state.registers.d = state.registers.b;
      break;
    case 0x51: /* LD D,C */
      state.registers.d = state.registers.c;
      break;
    case 0x52: /* LD D,D */
      state.registers.d = state.registers.d;
      break;
    case 0x53: /* LD D,E */
      state.registers.d = state.registers.e;
      break;
    case 0x54: /* LD D,H */
      state.registers.d = state.registers.h;
      break;
    case 0x55: /* LD D,L */
      state.registers.d = state.registers.l;
      break;
    case 0x56: /* LD D,(HL) */
      state.registers.d = bus->read8(state.registers.hl());
      cycle(1);
      break;
    case 0x57: /* LD D,A */
      state.registers.d = state.registers.a;
      break;
    case 0x58: /* LD E,B */
      state.registers.e = state.registers.b;
      break;
    case 0x59: /* LD E,C */
      state.registers.e = state.registers.c;
      break;
    case 0x5A: /* LD E,D */
      state.registers.e = state.registers.d;
      break;
    case 0x5B: /* LD E,E */
      state.registers.e = state.registers.e;
      break;
    case 0x5C: /* LD E,H */
      state.registers.e = state.registers.h;
      break;
    case 0x5D: /* LD E,L */
      state.registers.e = state.registers.l;
      break;
    case 0x5E: /* LD E,(HL) */
      state.registers.e = bus->read8(state.registers.hl());
      cycle(1);
      break;
    case 0x5F: /* LD E,A */
      state.registers.e = state.registers.a;
      break;
    case 0x60: /* LD H,B */
      state.registers.h = state.registers.b;
      break;
    case 0x61: /* LD H,C */
      state.registers.h = state.registers.c;
      break;
    case 0x62: /* LD H,D */
      state.registers.h = state.registers.d;
      break;
    case 0x63: /* LD H,E */
      state.registers.h = state.registers.e;
      break;
    case 0x64: /* LD H,H */
      state.registers.h = state.registers.h;
      break;
    case 0x65: /* LD H,L */
      state.registers.h = state.registers.l;
      break;
    case 0x66: /* LD H,(HL) */
      state.registers.h = bus->read8(state.registers.hl());
      cycle(1);
      break;
    case 0x67: /* LD H,A */
      state.registers.h = state.registers.a;
      break;
    case 0x68: /* LD L,B */
      state.registers.l = state.registers.b;
      break;
    case 0x69: /* LD L,C */
      state.registers.l = state.registers.c;
      break;
    case 0x6A: /* LD L,D */
      state.registers.l = state.registers.d;
      break;
    case 0x6B: /* LD L,E */
      state.registers.l = state.registers.e;
      break;
    case 0x6C: /* LD L,H */
      state.registers.l = state.registers.h;
      break;
    case 0x6D: /* LD L,L */
      state.registers.l = state.registers.l;
      break;
    case 0x6E: /* LD L,(HL) */
      state.registers.l = bus->read8(state.registers.hl());
      cycle(1);
      break;
    case 0x6F: /* LD L,A */
      state.registers.l = state.registers.a;
      break;
    case 0x70: /* LD (HL),B */
      bus->write8(state.registers.hl(), state.registers.b);
      cycle(1);
      break;
    case 0x71: /* LD (HL),C */
      bus->write8(state.registers.hl(), state.registers.c);
      cycle(1);
      break;
    case 0x72: /* LD (HL),D */
      bus->write8(state.registers.hl(), state.registers.d);
      cycle(1);
      break;
    case 0x73: /* LD (HL),E */
      bus->write8(state.registers.hl(), state.registers.e);
      cycle(1);
      break;
    case 0x74: /* LD (HL),H */
      bus->write8(state.registers.hl(), state.registers.h);
      cycle(1);
      break;
    case 0x75: /* LD (HL),L */
      bus->write8(state.registers.hl(), state.registers.l);
      cycle(1);
      break;

    case 0x76: { /* HALT */
      state.halted = true;
    } break;

    case 0x77: /* LD (HL),A */
      bus->write8(state.registers.hl(), state.registers.a);
      cycle(1);
      break;
    case 0x78: /* LD A,B */
      state.registers.a = state.registers.b;
      break;
    case 0x79: /* LD A,C */
      state.registers.a = state.registers.c;
      break;
    case 0x7A: /* LD A,D */
      state.registers.a = state.registers.d;
      break;
    case 0x7B: /* LD A,E */
      state.registers.a = state.registers.e;
      break;
    case 0x7C: /* LD A,H */
      state.registers.a = state.registers.h;
      break;
    case 0x7D: /* LD A,L */
      state.registers.a = state.registers.l;
      break;
    case 0x7E: /* LD A,(HL) */
      state.registers.a = bus->read8(state.registers.hl());
      cycle(1);
      break;
    case 0x7F: /* LD A,A */
      state.registers.a = state.registers.a;
      break;

    // ADD A,r  (0x80-0x87)
    case 0x80: { /* ADD A,B */
      uint16_t a = state.registers.a;
      uint16_t val = state.registers.b;
      state.registers.a = (a + val) & BYTE_MASK;
      setFlags(state.registers.a == 0, 0,
               (a & NIBBLE_MASK) + (val & NIBBLE_MASK) > NIBBLE_MASK,
               a + val > BYTE_MASK);
    } break;
    case 0x81: { /* ADD A,C */
      uint16_t a = state.registers.a;
      uint16_t val = state.registers.c;
      state.registers.a = (a + val) & BYTE_MASK;
      setFlags(state.registers.a == 0, 0,
               (a & NIBBLE_MASK) + (val & NIBBLE_MASK) > NIBBLE_MASK,
               a + val > BYTE_MASK);
    } break;
    case 0x82: { /* ADD A,D */
      uint16_t a = state.registers.a;
      uint16_t val = state.registers.d;
      state.registers.a = (a + val) & BYTE_MASK;
      setFlags(state.registers.a == 0, 0,
               (a & NIBBLE_MASK) + (val & NIBBLE_MASK) > NIBBLE_MASK,
               a + val > BYTE_MASK);
    } break;
    case 0x83: { /* ADD A,E */
      uint16_t a = state.registers.a;
      uint16_t val = state.registers.e;
      state.registers.a = (a + val) & BYTE_MASK;
      setFlags(state.registers.a == 0, 0,
               (a & NIBBLE_MASK) + (val & NIBBLE_MASK) > NIBBLE_MASK,
               a + val > BYTE_MASK);
    } break;
    case 0x84: { /* ADD A,H */
      uint16_t a = state.registers.a;
      uint16_t val = state.registers.h;
      state.registers.a = (a + val) & BYTE_MASK;
      setFlags(state.registers.a == 0, 0,
               (a & NIBBLE_MASK) + (val & NIBBLE_MASK) > NIBBLE_MASK,
               a + val > BYTE_MASK);
    } break;
    case 0x85: { /* ADD A,L */
      uint16_t a = state.registers.a;
      uint16_t val = state.registers.l;
      state.registers.a = (a + val) & BYTE_MASK;
      setFlags(state.registers.a == 0, 0,
               (a & NIBBLE_MASK) + (val & NIBBLE_MASK) > NIBBLE_MASK,
               a + val > BYTE_MASK);
    } break;
    case 0x86: { /* ADD A,(HL) */
      uint16_t a = state.registers.a;
      uint16_t val = bus->read8(state.registers.hl());
      cycle(1);
      state.registers.a = (a + val) & BYTE_MASK;
      setFlags(state.registers.a == 0, 0,
               (a & NIBBLE_MASK) + (val & NIBBLE_MASK) > NIBBLE_MASK,
               a + val > BYTE_MASK);
    } break;
    case 0x87: { /* ADD A,A */
      uint16_t a = state.registers.a;
      uint16_t val = state.registers.a;
      state.registers.a = (a + val) & BYTE_MASK;
      setFlags(state.registers.a == 0, 0,
               (a & NIBBLE_MASK) + (val & NIBBLE_MASK) > NIBBLE_MASK,
               a + val > BYTE_MASK);
    } break;

    // ADC A,r  (0x88-0x8F)
    case 0x88: { /* ADC A,B */
      uint16_t a = state.registers.a;
      uint16_t val = state.registers.b;
      uint16_t c = flagC();
      state.registers.a = (a + val + c) & BYTE_MASK;
      setFlags(state.registers.a == 0, 0,
               (a & NIBBLE_MASK) + (val & NIBBLE_MASK) + c > NIBBLE_MASK,
               a + val + c > BYTE_MASK);
    } break;
    case 0x89: { /* ADC A,C */
      uint16_t a = state.registers.a;
      uint16_t val = state.registers.c;
      uint16_t c = flagC();
      state.registers.a = (a + val + c) & BYTE_MASK;
      setFlags(state.registers.a == 0, 0,
               (a & NIBBLE_MASK) + (val & NIBBLE_MASK) + c > NIBBLE_MASK,
               a + val + c > BYTE_MASK);
    } break;
    case 0x8A: { /* ADC A,D */
      uint16_t a = state.registers.a;
      uint16_t val = state.registers.d;
      uint16_t c = flagC();
      state.registers.a = (a + val + c) & BYTE_MASK;
      setFlags(state.registers.a == 0, 0,
               (a & NIBBLE_MASK) + (val & NIBBLE_MASK) + c > NIBBLE_MASK,
               a + val + c > BYTE_MASK);
    } break;
    case 0x8B: { /* ADC A,E */
      uint16_t a = state.registers.a;
      uint16_t val = state.registers.e;
      uint16_t c = flagC();
      state.registers.a = (a + val + c) & BYTE_MASK;
      setFlags(state.registers.a == 0, 0,
               (a & NIBBLE_MASK) + (val & NIBBLE_MASK) + c > NIBBLE_MASK,
               a + val + c > BYTE_MASK);
    } break;
    case 0x8C: { /* ADC A,H */
      uint16_t a = state.registers.a;
      uint16_t val = state.registers.h;
      uint16_t c = flagC();
      state.registers.a = (a + val + c) & BYTE_MASK;
      setFlags(state.registers.a == 0, 0,
               (a & NIBBLE_MASK) + (val & NIBBLE_MASK) + c > NIBBLE_MASK,
               a + val + c > BYTE_MASK);
    } break;
    case 0x8D: { /* ADC A,L */
      uint16_t a = state.registers.a;
      uint16_t val = state.registers.l;
      uint16_t c = flagC();
      state.registers.a = (a + val + c) & BYTE_MASK;
      setFlags(state.registers.a == 0, 0,
               (a & NIBBLE_MASK) + (val & NIBBLE_MASK) + c > NIBBLE_MASK,
               a + val + c > BYTE_MASK);
    } break;
    case 0x8E: { /* ADC A,(HL) */
      uint16_t a = state.registers.a;
      uint16_t val = bus->read8(state.registers.hl());
      cycle(1);
      uint16_t c = flagC();
      state.registers.a = (a + val + c) & BYTE_MASK;
      setFlags(state.registers.a == 0, 0,
               (a & NIBBLE_MASK) + (val & NIBBLE_MASK) + c > NIBBLE_MASK,
               a + val + c > BYTE_MASK);
    } break;
    case 0x8F: { /* ADC A,A */
      uint16_t a = state.registers.a;
      uint16_t val = state.registers.a;
      uint16_t c = flagC();
      state.registers.a = (a + val + c) & BYTE_MASK;
      setFlags(state.registers.a == 0, 0,
               (a & NIBBLE_MASK) + (val & NIBBLE_MASK) + c > NIBBLE_MASK,
               a + val + c > BYTE_MASK);
    } break;

    // SUB A,r  (0x90-0x97)
    case 0x90: { /* SUB B */
      int a = state.registers.a;
      int val = state.registers.b;
      state.registers.a = (a - val) & BYTE_MASK;
      setFlags(state.registers.a == 0, 1,
               (a & NIBBLE_MASK) - (val & NIBBLE_MASK) < 0, a - val < 0);
    } break;
    case 0x91: { /* SUB C */
      int a = state.registers.a;
      int val = state.registers.c;
      state.registers.a = (a - val) & BYTE_MASK;
      setFlags(state.registers.a == 0, 1,
               (a & NIBBLE_MASK) - (val & NIBBLE_MASK) < 0, a - val < 0);
    } break;
    case 0x92: { /* SUB D */
      int a = state.registers.a;
      int val = state.registers.d;
      state.registers.a = (a - val) & BYTE_MASK;
      setFlags(state.registers.a == 0, 1,
               (a & NIBBLE_MASK) - (val & NIBBLE_MASK) < 0, a - val < 0);
    } break;
    case 0x93: { /* SUB E */
      int a = state.registers.a;
      int val = state.registers.e;
      state.registers.a = (a - val) & BYTE_MASK;
      setFlags(state.registers.a == 0, 1,
               (a & NIBBLE_MASK) - (val & NIBBLE_MASK) < 0, a - val < 0);
    } break;
    case 0x94: { /* SUB H */
      int a = state.registers.a;
      int val = state.registers.h;
      state.registers.a = (a - val) & BYTE_MASK;
      setFlags(state.registers.a == 0, 1,
               (a & NIBBLE_MASK) - (val & NIBBLE_MASK) < 0, a - val < 0);
    } break;
    case 0x95: { /* SUB L */
      int a = state.registers.a;
      int val = state.registers.l;
      state.registers.a = (a - val) & BYTE_MASK;
      setFlags(state.registers.a == 0, 1,
               (a & NIBBLE_MASK) - (val & NIBBLE_MASK) < 0, a - val < 0);
    } break;
    case 0x96: { /* SUB (HL) */
      int a = state.registers.a;
      int val = bus->read8(state.registers.hl());
      cycle(1);
      state.registers.a = (a - val) & BYTE_MASK;
      setFlags(state.registers.a == 0, 1,
               (a & NIBBLE_MASK) - (val & NIBBLE_MASK) < 0, a - val < 0);
    } break;
    case 0x97: { /* SUB A */
      int a = state.registers.a;
      int val = state.registers.a;
      state.registers.a = (a - val) & BYTE_MASK;
      setFlags(state.registers.a == 0, 1,
               (a & NIBBLE_MASK) - (val & NIBBLE_MASK) < 0, a - val < 0);
    } break;

    // SBC A,r  (0x98-0x9F)
    case 0x98: { /* SBC A,B */
      int a = state.registers.a;
      int val = state.registers.b;
      int c = flagC();
      state.registers.a = (a - val - c) & BYTE_MASK;
      setFlags(state.registers.a == 0, 1,
               (a & NIBBLE_MASK) - (val & NIBBLE_MASK) - c < 0,
               a - val - c < 0);
    } break;
    case 0x99: { /* SBC A,C */
      int a = state.registers.a;
      int val = state.registers.c;
      int c = flagC();
      state.registers.a = (a - val - c) & BYTE_MASK;
      setFlags(state.registers.a == 0, 1,
               (a & NIBBLE_MASK) - (val & NIBBLE_MASK) - c < 0,
               a - val - c < 0);
    } break;
    case 0x9A: { /* SBC A,D */
      int a = state.registers.a;
      int val = state.registers.d;
      int c = flagC();
      state.registers.a = (a - val - c) & BYTE_MASK;
      setFlags(state.registers.a == 0, 1,
               (a & NIBBLE_MASK) - (val & NIBBLE_MASK) - c < 0,
               a - val - c < 0);
    } break;
    case 0x9B: { /* SBC A,E */
      int a = state.registers.a;
      int val = state.registers.e;
      int c = flagC();
      state.registers.a = (a - val - c) & BYTE_MASK;
      setFlags(state.registers.a == 0, 1,
               (a & NIBBLE_MASK) - (val & NIBBLE_MASK) - c < 0,
               a - val - c < 0);
    } break;
    case 0x9C: { /* SBC A,H */
      int a = state.registers.a;
      int val = state.registers.h;
      int c = flagC();
      state.registers.a = (a - val - c) & BYTE_MASK;
      setFlags(state.registers.a == 0, 1,
               (a & NIBBLE_MASK) - (val & NIBBLE_MASK) - c < 0,
               a - val - c < 0);
    } break;
    case 0x9D: { /* SBC A,L */
      int a = state.registers.a;
      int val = state.registers.l;
      int c = flagC();
      state.registers.a = (a - val - c) & BYTE_MASK;
      setFlags(state.registers.a == 0, 1,
               (a & NIBBLE_MASK) - (val & NIBBLE_MASK) - c < 0,
               a - val - c < 0);
    } break;
    case 0x9E: { /* SBC A,(HL) */
      int a = state.registers.a;
      int val = bus->read8(state.registers.hl());
      cycle(1);
      int c = flagC();
      state.registers.a = (a - val - c) & BYTE_MASK;
      setFlags(state.registers.a == 0, 1,
               (a & NIBBLE_MASK) - (val & NIBBLE_MASK) - c < 0,
               a - val - c < 0);
    } break;
    case 0x9F: { /* SBC A,A */
      int a = state.registers.a;
      int val = state.registers.a;
      int c = flagC();
      state.registers.a = (a - val - c) & BYTE_MASK;
      setFlags(state.registers.a == 0, 1,
               (a & NIBBLE_MASK) - (val & NIBBLE_MASK) - c < 0,
               a - val - c < 0);
    } break;

    // AND A,r  (0xA0-0xA7)
    case 0xA0: /* AND B */
      state.registers.a &= state.registers.b;
      setFlags(state.registers.a == 0, 0, 1, 0);
      break;
    case 0xA1: /* AND C */
      state.registers.a &= state.registers.c;
      setFlags(state.registers.a == 0, 0, 1, 0);
      break;
    case 0xA2: /* AND D */
      state.registers.a &= state.registers.d;
      setFlags(state.registers.a == 0, 0, 1, 0);
      break;
    case 0xA3: /* AND E */
      state.registers.a &= state.registers.e;
      setFlags(state.registers.a == 0, 0, 1, 0);
      break;
    case 0xA4: /* AND H */
      state.registers.a &= state.registers.h;
      setFlags(state.registers.a == 0, 0, 1, 0);
      break;
    case 0xA5: /* AND L */
      state.registers.a &= state.registers.l;
      setFlags(state.registers.a == 0, 0, 1, 0);
      break;
    case 0xA6: /* AND (HL) */ {
      uint8_t v = bus->read8(state.registers.hl());
      cycle(1);
      state.registers.a &= v;
      setFlags(state.registers.a == 0, 0, 1, 0);
    } break;
    case 0xA7: /* AND A */
      state.registers.a &= state.registers.a;
      setFlags(state.registers.a == 0, 0, 1, 0);
      break;

    // XOR A,r  (0xA8-0xAF)
    case 0xA8: /* XOR B */
      state.registers.a ^= state.registers.b;
      setFlags(state.registers.a == 0, 0, 0, 0);
      break;
    case 0xA9: /* XOR C */
      state.registers.a ^= state.registers.c;
      setFlags(state.registers.a == 0, 0, 0, 0);
      break;
    case 0xAA: /* XOR D */
      state.registers.a ^= state.registers.d;
      setFlags(state.registers.a == 0, 0, 0, 0);
      break;
    case 0xAB: /* XOR E */
      state.registers.a ^= state.registers.e;
      setFlags(state.registers.a == 0, 0, 0, 0);
      break;
    case 0xAC: /* XOR H */
      state.registers.a ^= state.registers.h;
      setFlags(state.registers.a == 0, 0, 0, 0);
      break;
    case 0xAD: /* XOR L */
      state.registers.a ^= state.registers.l;
      setFlags(state.registers.a == 0, 0, 0, 0);
      break;
    case 0xAE: /* XOR (HL) */ {
      uint8_t v = bus->read8(state.registers.hl());
      cycle(1);
      state.registers.a ^= v;
      setFlags(state.registers.a == 0, 0, 0, 0);
    } break;
    case 0xAF: /* XOR A */
      state.registers.a ^= state.registers.a;
      setFlags(state.registers.a == 0, 0, 0, 0);
      break;

    // OR A,r  (0xB0-0xB7)
    case 0xB0: /* OR B */
      state.registers.a |= state.registers.b;
      setFlags(state.registers.a == 0, 0, 0, 0);
      break;
    case 0xB1: /* OR C */
      state.registers.a |= state.registers.c;
      setFlags(state.registers.a == 0, 0, 0, 0);
      break;
    case 0xB2: /* OR D */
      state.registers.a |= state.registers.d;
      setFlags(state.registers.a == 0, 0, 0, 0);
      break;
    case 0xB3: /* OR E */
      state.registers.a |= state.registers.e;
      setFlags(state.registers.a == 0, 0, 0, 0);
      break;
    case 0xB4: /* OR H */
      state.registers.a |= state.registers.h;
      setFlags(state.registers.a == 0, 0, 0, 0);
      break;
    case 0xB5: /* OR L */
      state.registers.a |= state.registers.l;
      setFlags(state.registers.a == 0, 0, 0, 0);
      break;
    case 0xB6: /* OR (HL) */ {
      uint8_t v = bus->read8(state.registers.hl());
      cycle(1);
      state.registers.a |= v;
      setFlags(state.registers.a == 0, 0, 0, 0);
    } break;
    case 0xB7: /* OR A */
      state.registers.a |= state.registers.a;
      setFlags(state.registers.a == 0, 0, 0, 0);
      break;

    // CP A,r  (0xB8-0xBF)
    case 0xB8: { /* CP B */
      int tmp = (int)state.registers.a - (int)state.registers.b;
      setFlags(tmp == 0, 1,
               (state.registers.a & NIBBLE_MASK) -
                       (state.registers.b & NIBBLE_MASK) <
                   0,
               tmp < 0);
    } break;
    case 0xB9: { /* CP C */
      int tmp = (int)state.registers.a - (int)state.registers.c;
      setFlags(tmp == 0, 1,
               (state.registers.a & NIBBLE_MASK) -
                       (state.registers.c & NIBBLE_MASK) <
                   0,
               tmp < 0);
    } break;
    case 0xBA: { /* CP D */
      int tmp = (int)state.registers.a - (int)state.registers.d;
      setFlags(tmp == 0, 1,
               (state.registers.a & NIBBLE_MASK) -
                       (state.registers.d & NIBBLE_MASK) <
                   0,
               tmp < 0);
    } break;
    case 0xBB: { /* CP E */
      int tmp = (int)state.registers.a - (int)state.registers.e;
      setFlags(tmp == 0, 1,
               (state.registers.a & NIBBLE_MASK) -
                       (state.registers.e & NIBBLE_MASK) <
                   0,
               tmp < 0);
    } break;
    case 0xBC: { /* CP H */
      int tmp = (int)state.registers.a - (int)state.registers.h;
      setFlags(tmp == 0, 1,
               (state.registers.a & NIBBLE_MASK) -
                       (state.registers.h & NIBBLE_MASK) <
                   0,
               tmp < 0);
    } break;
    case 0xBD: { /* CP L */
      int tmp = (int)state.registers.a - (int)state.registers.l;
      setFlags(tmp == 0, 1,
               (state.registers.a & NIBBLE_MASK) -
                       (state.registers.l & NIBBLE_MASK) <
                   0,
               tmp < 0);
    } break;
    case 0xBE: { /* CP (HL) */
      uint8_t val = bus->read8(state.registers.hl());
      cycle(1);
      int tmp = (int)state.registers.a - (int)val;
      setFlags(
          tmp == 0, 1,
          ((int)state.registers.a & NIBBLE_MASK) - ((int)val & NIBBLE_MASK) < 0,
          tmp < 0);
    } break;
    case 0xBF: { /* CP A */
      int tmp = (int)state.registers.a - (int)state.registers.a;
      setFlags(tmp == 0, 1,
               (state.registers.a & NIBBLE_MASK) -
                       (state.registers.a & NIBBLE_MASK) <
                   0,
               tmp < 0);
    } break;

    case 0xC0: { /* RET NZ */
      cycle(1);
      if (!flagZ()) {
        uint16_t lo = stackPop8();
        cycle(1);
        uint16_t hi = stackPop8();
        cycle(1);
        state.registers.pc = (hi << 8) | lo;
        cycle(1);
      }
    } break;

    case 0xC1: { /* POP BC */
      uint16_t lo = stackPop8();
      cycle(1);
      uint16_t hi = stackPop8();
      cycle(1);
      state.registers.setBc((hi << 8) | lo);
    } break;

    case 0xC2: { /* JP NZ,a16 */
      uint16_t lo = bus->read8(state.registers.pc);
      cycle(1);
      uint16_t hi = bus->read8(state.registers.pc + 1);
      cycle(1);
      state.registers.pc += 2;
      if (!flagZ()) {
        state.registers.pc = lo | (hi << 8);
        cycle(1);
      }
    } break;

    case 0xC3: { /* JP a16 */
      uint16_t lo = bus->read8(state.registers.pc);
      cycle(1);
      uint16_t hi = bus->read8(state.registers.pc + 1);
      cycle(1);
      state.registers.pc = lo | (hi << 8);
      cycle(1);
    } break;

    case 0xC4: { /* CALL NZ,a16 */
      uint16_t lo = bus->read8(state.registers.pc);
      cycle(1);
      uint16_t hi = bus->read8(state.registers.pc + 1);
      cycle(1);
      state.registers.pc += 2;
      if (!flagZ()) {
        cycle(2);
        stackPush16(state.registers.pc);
        state.registers.pc = lo | (hi << 8);
        cycle(1);
      }
    } break;

    case 0xC5: { /* PUSH BC */
      uint16_t hi = (state.registers.bc() >> 8) & BYTE_MASK;
      cycle(1);
      stackPush8(hi);
      uint16_t lo = state.registers.bc() & BYTE_MASK;
      cycle(1);
      stackPush8(lo);
      cycle(1);
    } break;

    case 0xC6: { /* ADD A,d8 */
      uint16_t val = bus->read8(state.registers.pc++);
      cycle(1);
      uint16_t a = state.registers.a;
      state.registers.a = (a + val) & BYTE_MASK;
      setFlags(state.registers.a == 0, 0,
               (a & NIBBLE_MASK) + (val & NIBBLE_MASK) > NIBBLE_MASK,
               a + val > BYTE_MASK);
    } break;

    case 0xC7: { /* RST 00H */
      cycle(2);
      stackPush16(state.registers.pc);
      state.registers.pc = 0x00;
      cycle(1);
    } break;

    case 0xC8: { /* RET Z */
      cycle(1);
      if (flagZ()) {
        uint16_t lo = stackPop8();
        cycle(1);
        uint16_t hi = stackPop8();
        cycle(1);
        state.registers.pc = (hi << 8) | lo;
        cycle(1);
      }
    } break;

    case 0xC9: { /* RET */
      uint16_t lo = stackPop8();
      cycle(1);
      uint16_t hi = stackPop8();
      cycle(1);
      state.registers.pc = (hi << 8) | lo;
      cycle(1);
    } break;

    case 0xCA: { /* JP Z,a16 */
      uint16_t lo = bus->read8(state.registers.pc);
      cycle(1);
      uint16_t hi = bus->read8(state.registers.pc + 1);
      cycle(1);
      state.registers.pc += 2;
      if (flagZ()) {
        state.registers.pc = lo | (hi << 8);
        cycle(1);
      }
    } break;

    case 0xCB: { /* CB prefix */
      executeCB();
    } break;

    case 0xCC: { /* CALL Z,a16 */
      uint16_t lo = bus->read8(state.registers.pc);
      cycle(1);
      uint16_t hi = bus->read8(state.registers.pc + 1);
      cycle(1);
      state.registers.pc += 2;
      if (flagZ()) {
        cycle(2);
        stackPush16(state.registers.pc);
        state.registers.pc = lo | (hi << 8);
        cycle(1);
      }
    } break;

    case 0xCD: { /* CALL a16 */
      uint16_t lo = bus->read8(state.registers.pc);
      cycle(1);
      uint16_t hi = bus->read8(state.registers.pc + 1);
      cycle(1);
      state.registers.pc += 2;
      cycle(2);
      stackPush16(state.registers.pc);
      state.registers.pc = lo | (hi << 8);
      cycle(1);
    } break;

    case 0xCE: { /* ADC A,d8 */
      uint16_t val = bus->read8(state.registers.pc++);
      cycle(1);
      uint16_t a = state.registers.a;
      uint16_t c = flagC();
      state.registers.a = (a + val + c) & BYTE_MASK;
      setFlags(state.registers.a == 0, 0,
               (a & NIBBLE_MASK) + (val & NIBBLE_MASK) + c > NIBBLE_MASK,
               a + val + c > BYTE_MASK);
    } break;

    case 0xCF: { /* RST 08H */
      cycle(2);
      stackPush16(state.registers.pc);
      state.registers.pc = 0x08;
      cycle(1);
    } break;

    case 0xD0: { /* RET NC */
      cycle(1);
      if (!flagC()) {
        uint16_t lo = stackPop8();
        cycle(1);
        uint16_t hi = stackPop8();
        cycle(1);
        state.registers.pc = (hi << 8) | lo;
        cycle(1);
      }
    } break;

    case 0xD1: { /* POP DE */
      uint16_t lo = stackPop8();
      cycle(1);
      uint16_t hi = stackPop8();
      cycle(1);
      state.registers.setDe((hi << 8) | lo);
    } break;

    case 0xD2: { /* JP NC,a16 */
      uint16_t lo = bus->read8(state.registers.pc);
      cycle(1);
      uint16_t hi = bus->read8(state.registers.pc + 1);
      cycle(1);
      state.registers.pc += 2;
      if (!flagC()) {
        state.registers.pc = lo | (hi << 8);
        cycle(1);
      }
    } break;

    case 0xD3: { /* UNUSED */
      throw std::runtime_error("Unknown Instruction Type");
    } break;

    case 0xD4: { /* CALL NC,a16 */
      uint16_t lo = bus->read8(state.registers.pc);
      cycle(1);
      uint16_t hi = bus->read8(state.registers.pc + 1);
      cycle(1);
      state.registers.pc += 2;
      if (!flagC()) {
        cycle(2);
        stackPush16(state.registers.pc);
        state.registers.pc = lo | (hi << 8);
        cycle(1);
      }
    } break;

    case 0xD5: { /* PUSH DE */
      uint16_t hi = (state.registers.de() >> 8) & BYTE_MASK;
      cycle(1);
      stackPush8(hi);
      uint16_t lo = state.registers.de() & BYTE_MASK;
      cycle(1);
      stackPush8(lo);
      cycle(1);
    } break;

    case 0xD6: { /* SUB d8 */
      uint8_t val = bus->read8(state.registers.pc++);
      cycle(1);
      int a = state.registers.a;
      state.registers.a = (a - val) & BYTE_MASK;
      setFlags(state.registers.a == 0, 1,
               (a & NIBBLE_MASK) - (val & NIBBLE_MASK) < 0, a - val < 0);
    } break;

    case 0xD7: { /* RST 10H */
      cycle(2);
      stackPush16(state.registers.pc);
      state.registers.pc = 0x10;
      cycle(1);
    } break;

    case 0xD8: { /* RET C */
      cycle(1);
      if (flagC()) {
        uint16_t lo = stackPop8();
        cycle(1);
        uint16_t hi = stackPop8();
        cycle(1);
        state.registers.pc = (hi << 8) | lo;
        cycle(1);
      }
    } break;

    case 0xD9: { /* RETI */
      state.ime = true;
      uint16_t lo = stackPop8();
      cycle(1);
      uint16_t hi = stackPop8();
      cycle(1);
      state.registers.pc = (hi << 8) | lo;
      cycle(1);
    } break;

    case 0xDA: { /* JP C,a16 */
      uint16_t lo = bus->read8(state.registers.pc);
      cycle(1);
      uint16_t hi = bus->read8(state.registers.pc + 1);
      cycle(1);
      state.registers.pc += 2;
      if (flagC()) {
        state.registers.pc = lo | (hi << 8);
        cycle(1);
      }
    } break;

    case 0xDB: { /* UNUSED */
      throw std::runtime_error("Unknown Instruction Type");
    } break;

    case 0xDC: { /* CALL C,a16 */
      uint16_t lo = bus->read8(state.registers.pc);
      cycle(1);
      uint16_t hi = bus->read8(state.registers.pc + 1);
      cycle(1);
      state.registers.pc += 2;
      if (flagC()) {
        cycle(2);
        stackPush16(state.registers.pc);
        state.registers.pc = lo | (hi << 8);
        cycle(1);
      }
    } break;

    case 0xDD: { /* UNUSED */
      throw std::runtime_error("Unknown Instruction Type");
    } break;

    case 0xDE: { /* SBC A,d8 */
      uint8_t val = bus->read8(state.registers.pc++);
      cycle(1);
      int a = state.registers.a;
      int c = flagC();
      state.registers.a = (a - val - c) & BYTE_MASK;
      setFlags(state.registers.a == 0, 1,
               (a & NIBBLE_MASK) - (val & NIBBLE_MASK) - c < 0,
               a - val - c < 0);
    } break;

    case 0xDF: { /* RST 18H */
      cycle(2);
      stackPush16(state.registers.pc);
      state.registers.pc = 0x18;
      cycle(1);
    } break;

    case 0xE0: { /* LDH (a8),A */
      uint8_t offset = bus->read8(state.registers.pc++);
      cycle(1);
      bus->write8(IO_REGISTERS_START | offset, state.registers.a);
      cycle(1);
    } break;

    case 0xE1: { /* POP HL */
      uint16_t lo = stackPop8();
      cycle(1);
      uint16_t hi = stackPop8();
      cycle(1);
      state.registers.setHl((hi << 8) | lo);
    } break;

    case 0xE2: { /* LD (C),A */
      bus->write8(IO_REGISTERS_START | state.registers.c, state.registers.a);
      cycle(1);
    } break;

    case 0xE3: { /* UNUSED */
      throw std::runtime_error("Unknown Instruction Type");
    } break;

    case 0xE4: { /* UNUSED */
      throw std::runtime_error("Unknown Instruction Type");
    } break;

    case 0xE5: { /* PUSH HL */
      uint16_t hi = (state.registers.hl() >> 8) & BYTE_MASK;
      cycle(1);
      stackPush8(hi);
      uint16_t lo = state.registers.hl() & BYTE_MASK;
      cycle(1);
      stackPush8(lo);
      cycle(1);
    } break;

    case 0xE6: { /* AND d8 */
      uint8_t val = bus->read8(state.registers.pc++);
      cycle(1);
      state.registers.a &= val;
      setFlags(state.registers.a == 0, 0, 1, 0);
    } break;

    case 0xE7: { /* RST 20H */
      cycle(2);
      stackPush16(state.registers.pc);
      state.registers.pc = 0x20;
      cycle(1);
    } break;

    case 0xE8: { /* ADD SP,r8 */
      int8_t val = (int8_t)bus->read8(state.registers.pc++);
      cycle(1);
      int h = (state.registers.sp & NIBBLE_MASK) + (val & NIBBLE_MASK) >= 0x10;
      int c = (int)(state.registers.sp & BYTE_MASK) + (int)(val & BYTE_MASK) >=
              0x100;
      state.registers.sp = (uint16_t)((int)state.registers.sp + val);
      cycle(1);
      setFlags(0, 0, h, c);
    } break;

    case 0xE9: { /* JP HL */
      state.registers.pc = state.registers.hl();
    } break;

    case 0xEA: { /* LD (a16),A */
      uint16_t lo = bus->read8(state.registers.pc);
      cycle(1);
      uint16_t hi = bus->read8(state.registers.pc + 1);
      cycle(1);
      state.registers.pc += 2;
      bus->write8(lo | (hi << 8), state.registers.a);
      cycle(1);
    } break;

    case 0xEB: { /* UNUSED */
      throw std::runtime_error("Unknown Instruction Type");
    } break;

    case 0xEC: { /* UNUSED */
      throw std::runtime_error("Unknown Instruction Type");
    } break;

    case 0xED: { /* UNUSED */
      throw std::runtime_error("Unknown Instruction Type");
    } break;

    case 0xEE: { /* XOR d8 */
      uint8_t val = bus->read8(state.registers.pc++);
      cycle(1);
      state.registers.a ^= val;
      setFlags(state.registers.a == 0, 0, 0, 0);
    } break;

    case 0xEF: { /* RST 28H */
      cycle(2);
      stackPush16(state.registers.pc);
      state.registers.pc = 0x28;
      cycle(1);
    } break;

    case 0xF0: { /* LDH A,(a8) */
      uint8_t offset = bus->read8(state.registers.pc++);
      cycle(1);
      state.registers.a = bus->read8(IO_REGISTERS_START | offset);
      cycle(1);
    } break;

    case 0xF1: { /* POP AF */
      uint16_t lo = stackPop8();
      cycle(1);
      uint16_t hi = stackPop8();
      cycle(1);
      state.registers.setAf((hi << 8) | (lo & HIGH_NIBBLE_MASK));
    } break;

    case 0xF2: { /* LD A,(C) */
      state.registers.a = bus->read8(IO_REGISTERS_START | state.registers.c);
      cycle(1);
    } break;

    case 0xF3: { /* DI */
      state.ime = false;
    } break;

    case 0xF4: { /* UNUSED */
      throw std::runtime_error("Unknown Instruction Type");
    } break;

    case 0xF5: { /* PUSH AF */
      uint16_t hi = (state.registers.af() >> 8) & BYTE_MASK;
      cycle(1);
      stackPush8(hi);
      uint16_t lo = state.registers.af() & BYTE_MASK;
      cycle(1);
      stackPush8(lo);
      cycle(1);
    } break;

    case 0xF6: { /* OR d8 */
      uint8_t val = bus->read8(state.registers.pc++);
      cycle(1);
      state.registers.a |= val;
      setFlags(state.registers.a == 0, 0, 0, 0);
    } break;

    case 0xF7: { /* RST 30H */
      cycle(2);
      stackPush16(state.registers.pc);
      state.registers.pc = 0x30;
      cycle(1);
    } break;

    case 0xF8: { /* LD HL,SP+r8 */
      int8_t val = (int8_t)bus->read8(state.registers.pc++);
      cycle(1);
      uint8_t H =
          (state.registers.sp & NIBBLE_MASK) + (val & NIBBLE_MASK) >= 0x10;
      uint8_t C = (state.registers.sp & BYTE_MASK) + (val & BYTE_MASK) >= 0x100;
      setFlags(0, 0, H, C);
      state.registers.setHl((uint16_t)((int)state.registers.sp + val));
    } break;

    case 0xF9: { /* LD SP,HL */
      state.registers.sp = state.registers.hl();
      cycle(1);
    } break;

    case 0xFA: { /* LD A,(a16) */
      uint16_t lo = bus->read8(state.registers.pc);
      cycle(1);
      uint16_t hi = bus->read8(state.registers.pc + 1);
      cycle(1);
      state.registers.a = bus->read8(lo | (hi << 8));
      cycle(1);
      state.registers.pc += 2;
    } break;

    case 0xFB: { /* EI */
      state.imeScheduled = true;
    } break;

    case 0xFC: { /* UNUSED */
      throw std::runtime_error("Unknown Instruction Type");
    } break;

    case 0xFD: { /* UNUSED */
      throw std::runtime_error("Unknown Instruction Type");
    } break;

    case 0xFE: { /* CP d8 */
      uint8_t val = bus->read8(state.registers.pc++);
      cycle(1);
      int tmp = (int)state.registers.a - (int)val;
      setFlags(
          tmp == 0, 1,
          ((int)state.registers.a & NIBBLE_MASK) - ((int)val & NIBBLE_MASK) < 0,
          tmp < 0);
    } break;

    case 0xFF: { /* RST 38H */
      cycle(2);
      stackPush16(state.registers.pc);
      state.registers.pc = 0x38;
      cycle(1);
    } break;
    }
  }

  if (state.ime) [[unlikely]] {
    handleInterrupts();
    state.imeScheduled = false;
  }

  if (state.imeScheduled) {
    state.ime = true;
  }
}
