#include "Machine.h"

#include <cstdint>
#include <string_view>
#include <vector>

#include "Common.h"

Machine::Machine(std::string_view romPath)
    : cartridge_(romPath),
      cpu_(&Machine::cycleCallback, this, nullptr),
      lcd_([this](uint8_t value) { dma_.start(value); }),
      ppu_(cpu_.getInterruptRegs()),
      dma_([this](uint16_t addr, uint8_t val) { ppu_.oamWrite(addr, val); }),
      timer_(cpu_.getInterruptRegs()),
      io_(cpu_.getInterruptRegs(), timer_, lcd_, gamepad_, apu_),
      bus_(cartridge_, cpu_.getInterruptRegs(), dma_, io_, ppu_, ram_),
      serializer_(std::vector<StateSerializer::Section>{
          {.name = "CPU", .component = &cpu_},
          {.name = "RAM", .component = &ram_},
          {.name = "PPU", .component = &ppu_},
          {.name = "LCD", .component = &lcd_},
          {.name = "TIMER", .component = &timer_},
          {.name = "DMA", .component = &dma_},
          {.name = "APU", .component = &apu_},
          {.name = "CARTRIDGE", .component = &cartridge_},
          {.name = "JOYPAD", .component = &gamepad_}}) {
  cpu_.setMemory(&bus_);
  dma_.setMemRead(bus_);
  ppu_.setLcd(&lcd_);
  ppu_.init();
}

void Machine::cycleCallback(void* self, int cycles) {
  static_cast<Machine*>(self)->cycle(cycles);
}

void Machine::cycle(int cycles) {
  cycles_ += static_cast<uint64_t>(cycles);
  cartridge_.tick(cycles);
  for (int i = 0; i < cycles; i++) {
    for (int j = 0; j < CLOCK_CYCLES; j++) {
      timer_.tick();
      ppu_.tick();
      apu_.tick();
    }
    dma_.tick();
  }
}
