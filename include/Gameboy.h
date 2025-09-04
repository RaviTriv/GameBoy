#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

class APU;
class Bus;
class Cartridge;
class CPU;
class DMA;
class IO;
class LCD;
class PPU;
class RAM;
class StateSerializer;
class Timer;
class Gamepad;
class UI;
class GameBoy
{
public:
  struct State
  {
    bool isRunning;
    bool isPaused;
    uint64_t ticks;
  };
  void init(std::string romPath, bool trace, bool loadSave, bool fastForward);
  void run();

  void cycle(int cycles);

private:
  State state;
  std::shared_ptr<APU> apu;
  std::shared_ptr<Bus> bus;
  std::shared_ptr<Cartridge> cartridge;
  std::shared_ptr<CPU> cpu;
  std::shared_ptr<DMA> dma;
  std::shared_ptr<IO> io;
  std::shared_ptr<LCD> lcd;
  std::shared_ptr<PPU> ppu;
  std::shared_ptr<RAM> ram;
  std::shared_ptr<Timer> timer;
  std::shared_ptr<Gamepad> gamepad;
  std::shared_ptr<UI> ui;
  std::shared_ptr<StateSerializer> stateSerializer;

  std::thread cpuThread;
  void cpuLoop();
  void displayBootArt();
  void saveState();
  bool trace;
  bool loadSave;
  bool fastForward;
};