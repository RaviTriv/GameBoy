#pragma once

#include <cstdint>
#include <memory>
#include <string>

class Bus;
class Cartridge;
class CPU;
class GameBoy : public std::enable_shared_from_this<GameBoy>
{
public:
  struct State
  {
    bool isRunning;
    bool isPaused;
    uint64_t ticks;
  };
  GameBoy();
  void init(std::string romPath);
  void run();

private:
  State state;
  std::shared_ptr<Bus> bus;
  std::shared_ptr<Cartridge> cartridge;
  std::shared_ptr<CPU> cpu;
};