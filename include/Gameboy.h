#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <variant>

#include "spsc_queue.hpp"

class APU;
class Bus;
class Cartridge;
class CPU;
class DMA;
class FramePacer;
class IO;
class LCD;
class PPU;
class RAM;
class StateSerializer;
class Timer;
class Gamepad;
class UI;
class GameBoy {
 public:
  struct State {
    std::atomic<bool> isRunning{false};
    bool isPaused = false;
    uint64_t ticks = 0;
  };
  GameBoy();
  GameBoy(const GameBoy&) = delete;
  GameBoy& operator=(const GameBoy&) = delete;
  ~GameBoy();
  void init(const std::string& romPath, bool enableTrace, bool loadSaveOnStart,
            bool startFastForward);
  void run();

  void cycle(int cycles);

 private:
  struct SaveStateCommand {};
  struct ToggleFastForwardCommand {};
  using Command = std::variant<SaveStateCommand, ToggleFastForwardCommand>;

  State state;
  spsc::Queue<Command, 64> commands;
  std::unique_ptr<APU> apu;
  std::unique_ptr<Bus> bus;
  std::unique_ptr<Cartridge> cartridge;
  std::unique_ptr<CPU> cpu;
  std::unique_ptr<DMA> dma;
  std::unique_ptr<IO> io;
  std::unique_ptr<LCD> lcd;
  std::unique_ptr<PPU> ppu;
  std::unique_ptr<RAM> ram;
  std::unique_ptr<Timer> timer;
  std::unique_ptr<Gamepad> gamepad;
  std::unique_ptr<UI> ui;
  std::unique_ptr<StateSerializer> stateSerializer;
  std::unique_ptr<FramePacer> framePacer;

  std::thread cpuThread;
  void cpuLoop();
  void pushCommand(const Command& command);
  void processCommands();
  void displayBootArt();
  void saveState();
  bool trace = false;
  bool loadSave = false;
  bool fastForward = false;
};
