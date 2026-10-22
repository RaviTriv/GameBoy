// clang-format Language: Cpp
#pragma once

#include <cstdint>
#include <functional>
#include <string_view>
#include <utility>
#include <vector>

#include "Apu.h"
#include "Bus.h"
#include "Cartridge.h"
#include "Cpu.h"
#include "Dma.h"
#include "Gamepad.h"
#include "IMemRead.h"
#include "Interrupts.h"
#include "Io.h"
#include "Lcd.h"
#include "Ppu.h"
#include "Ram.h"
#include "Serializable.h"
#include "StateSerializer.h"
#include "Timer.h"

class Machine {
 public:
  explicit Machine(std::string_view romPath);
  Machine(const Machine&) = delete ("components hold pointers to each other");
  Machine& operator=(const Machine&) =
      delete ("components hold pointers to each other");
  Machine(Machine&&) = delete ("components hold pointers to each other");
  Machine& operator=(Machine&&) =
      delete ("components hold pointers to each other");
  ~Machine() = default;

  void step() { cpu_.step(); }

  void setOnFrameComplete(std::function<void()> callback) {
    ppu_.setOnFrameComplete(std::move(callback));
  }
  void setSerialSink(std::function<void(uint8_t)> sink) {
    io_.setSerialSink(std::move(sink));
  }

  [[nodiscard]] uint64_t cycles() const { return cycles_; }
  [[nodiscard]] Cartridge& cartridge() { return cartridge_; }
  [[nodiscard]] Gamepad& gamepad() { return gamepad_; }
  [[nodiscard]] APU& apu() { return apu_; }
  [[nodiscard]] PPU& ppu() { return ppu_; }
  [[nodiscard]] const PPU& ppu() const { return ppu_; }
  [[nodiscard]] const LCD& lcd() const { return lcd_; }
  [[nodiscard]] const CPU& cpu() const { return cpu_; }
  [[nodiscard]] IMemRead& memory() { return bus_; }
  [[nodiscard]] StateSerializer& serializer() { return serializer_; }

 private:
  void cycle(int cycles);
  std::vector<StateSerializer::Section> saveSections();

  Interrupts interrupts_;
  Cartridge cartridge_;
  RAM ram_;
  Gamepad gamepad_;
  APU apu_;
  DMA dma_;
  LCD lcd_;
  PPU ppu_;
  Timer timer_;
  IO io_;
  [[= serial::skip]] Bus bus_;
  CPU cpu_;
  [[= serial::skip]] StateSerializer serializer_;
  [[= serial::skip]] uint64_t cycles_ = 0;
};
