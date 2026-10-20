#include "Machine.h"

#include <cstdint>
#include <string_view>
#include <vector>

#include "Common.h"

Machine::Machine(std::string_view romPath)
    : cartridge_(romPath),
      lcd_(dma_),
      ppu_(interrupts_, lcd_),
      timer_(interrupts_),
      io_(interrupts_, timer_, lcd_, gamepad_, apu_),
      bus_(cartridge_, interrupts_, dma_, io_, ppu_, ram_),
      cpu_(interrupts_, bus_, &Machine::cycleCallback, this),
      serializer_(std::vector<StateSerializer::Section>{
          {.name = "CPU", .component = &cpu_},
          {.name = "INTERRUPTS", .component = &interrupts_},
          {.name = "RAM", .component = &ram_},
          {.name = "PPU", .component = &ppu_},
          {.name = "LCD", .component = &lcd_},
          {.name = "TIMER", .component = &timer_},
          {.name = "DMA", .component = &dma_},
          {.name = "APU", .component = &apu_},
          {.name = "CARTRIDGE", .component = &cartridge_},
          {.name = "JOYPAD", .component = &gamepad_}}) {}

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
    dma_.tick(bus_, ppu_);
  }
}
