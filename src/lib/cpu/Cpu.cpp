#include "../../../include/Cpu.h"
#include "../../../include/Bus.h"
#include "../../../include/Logger.h"

CPU::CPU(CycleCallback cycleCallback, std::shared_ptr<Bus> bus) : cycleCallback(cycleCallback), bus(bus)
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
  state.currentOpcode = bus->read(state.registers.pc);
}

void CPU::decode()
{
  decoder.decode(state.currentOpcode);
}

void CPU::step()
{
  fetch();
}