#include "Cpu.h"
#include "Bus.h"
#include "Logger.h"

CPU::CPU(CycleCallback cycleCallback, Bus *bus)
    : cycleCallback(cycleCallback), bus(bus),
      context({state.registers, state.instruction, state.opcode, state.opValue,
               state.isMemoryOp, state.memoryAddress, state.ime,
               state.imeScheduled, state.ie, state.intf, state.halted},
              this->cycleCallback, this->bus),
      decoder(&context), executer(&context), interrupt(&context) {
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
};

void CPU::setBus(Bus *bus) { this->bus = bus; }

void CPU::fetch() { state.opcode = bus->read8(state.registers.pc++); }

void CPU::decode() { decoder.decode(state.opcode); }

void CPU::execute() { executer.execute(); }

void CPU::step() {
  if (!state.halted) {
    fetch();
    decode();
    execute();
  } else {
    cycleCallback(1);
    if (state.intf) {
      state.halted = false;
    }
  }

  if (state.ime) {
    interrupt.handleInterrupts();
    state.imeScheduled = false;
  }

  if (state.imeScheduled) {
    state.ime = true;
  }
}

void CPU::requestInterrupt(InterruptType type) {
  interrupt.requestInterrupt(type);
}

void CPU::setInterruptEnable(uint8_t value) {
  interrupt.setInterruptEnable(value);
}

uint8_t CPU::getInterruptEnable() const {
  return interrupt.getInterruptEnable();
}

void CPU::setInterruptFlags(uint8_t value) { state.intf = value; }

uint8_t CPU::getInterruptFlags() const { return state.intf; }

CPU::State CPU::getState() const { return state; }

void CPU::setState(const State &state) { this->state = state; }

InterruptRegs CPU::getInterruptRegs() { return {state.ie, state.intf}; }
