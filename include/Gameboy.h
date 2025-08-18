#pragma once

#include <cstdint>
#include <memory>
#include <string>

struct GameBoyState
{
  bool isRunning;
  bool isPaused;
  uint64_t ticks;
};

class Cartridge;
class CPU;
class GameBoy : public std::enable_shared_from_this<GameBoy>
{
public:
  GameBoy();
  void init(std::string romPath);
  void run();

private:
  GameBoyState gameBoyState;
  std::shared_ptr<Cartridge> cartridge;
  std::shared_ptr<CPU> cpu;
};