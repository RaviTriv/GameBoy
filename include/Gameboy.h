#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

class Bus;
class Cartridge;
class CPU;
class DMA;
class IO;
class LCD;
class PPU;
class RAM;
class Timer;
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
  void init(std::string romPath);
  void run();

  void cycle(int cycles);

private:
  State state;
  std::shared_ptr<Bus> bus;
  std::shared_ptr<Cartridge> cartridge;
  std::shared_ptr<CPU> cpu;
  std::shared_ptr<DMA> dma;
  std::shared_ptr<IO> io;
  std::shared_ptr<LCD> lcd;
  std::shared_ptr<PPU> ppu;
  std::shared_ptr<RAM> ram;
  std::shared_ptr<Timer> timer;
  std::shared_ptr<UI> ui;

  std::thread cpuThread;
  void cpuLoop();
};