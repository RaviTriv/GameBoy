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
  GameBoy();
  ~GameBoy();
  void init(std::string romPath, bool trace, bool loadSave, bool fastForward);
  void run();

  void cycle(int cycles);

private:
  State state;
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

  std::thread cpuThread;
  void cpuLoop();
  void displayBootArt();
  void saveState();
  bool trace;
  bool loadSave;
  bool fastForward;
};