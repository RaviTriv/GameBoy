#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <variant>

#include "SpscQueue.h"

class FramePacer;
class Machine;
class UI;
class GameBoy {
 public:
  struct State {
    std::atomic<bool> isRunning{false};
    bool isPaused = false;
  };
  GameBoy();
  GameBoy(const GameBoy&) = delete;
  GameBoy& operator=(const GameBoy&) = delete;
  ~GameBoy();
  void init(const std::string& romPath, bool enableTrace, bool loadSaveOnStart,
            bool startFastForward);
  void run();

 private:
  struct SaveStateCommand {};
  struct ToggleFastForwardCommand {};
  using Command = std::variant<SaveStateCommand, ToggleFastForwardCommand>;

  State state;
  SPSCQueue<Command, 64> commands;
  std::unique_ptr<Machine> machine;
  std::unique_ptr<UI> ui;
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
