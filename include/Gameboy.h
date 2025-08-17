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

class GameBoy : public std::enable_shared_from_this<GameBoy>
{
public:
  void init(std::string romPath);
  void run();

private:
  GameBoyState gameBoyState;
};