#include "Gameboy.h"

#include <spdlog/common.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <variant>

#include "Common.h"
#include "FramePacer.h"
#include "Logger.h"
#include "Machine.h"
#include "StateSerializer.h"
#include "Ui.h"

namespace {
template <class... Ts>
struct Overloaded : Ts... {
  using Ts::operator()...;
};
}  // namespace

GameBoy::GameBoy() = default;
GameBoy::~GameBoy() {
  state.isRunning.store(false);
  if (cpuThread.joinable()) {
    cpuThread.join();
  }
}

void GameBoy::init(const std::string& romPath, bool enableTrace,
                   bool loadSaveOnStart, bool startFastForward) {
  Logger::GetLogger()->set_level(spdlog::level::off);
  if (enableTrace) {
    Logger::GetLogger()->set_level(spdlog::level::trace);
  }
  displayBootArt();
  Logger::GetLogger()->info("Initializing GameBoy");
  machine = std::make_unique<Machine>(romPath);
  StateSerializer::loadBattery(machine->cartridge());
  ui = std::make_unique<UI>([this] { state.isRunning.store(false); },
                            [this] { pushCommand(SaveStateCommand{}); },
                            machine->ppu(), machine->gamepad(), machine->apu());
  framePacer =
      std::make_unique<FramePacer>([this] { return ui->getTicks(); },
                                   [this](uint32_t ms) { ui->delay(ms); });
  framePacer->setFastForward(startFastForward);
  machine->setOnFrameComplete([this] { framePacer->onFrameComplete(); });
  ui->setToggleFastForward([this] { pushCommand(ToggleFastForwardCommand{}); });
  state.isRunning.store(true);
  trace = enableTrace;
  loadSave = loadSaveOnStart;
  fastForward = startFastForward;
}

void GameBoy::saveState() {
  const std::string title = machine->cartridge().getTitle();
  if (!machine->serializer().saveState(title)) {
    Logger::GetLogger()->error("Failed to save state for: {}", title);
  }
}

void GameBoy::run() {
  ui->init();

  const std::string title = machine->cartridge().getTitle();
  if (loadSave && (!machine->serializer().loadState(title))) {
    Logger::GetLogger()->error("Failed to load save state for: {}", title);
  }

  cpuThread = std::thread(&GameBoy::cpuLoop, this);

  uint32_t prevFrame = 0;
  while (state.isRunning.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(UI_THREAD_SLEEP_MS));
    ui->handleEvents();
    if (prevFrame != machine->ppu().getCurrentFrame()) {
      ui->update();
    }
    prevFrame = machine->ppu().getCurrentFrame();
  }

  cpuThread.join();
  StateSerializer::saveBattery(machine->cartridge());
}

void GameBoy::cpuLoop() {
  while (state.isRunning.load()) {
    processCommands();
    machine->step();
  }
}

void GameBoy::pushCommand(const Command& command) {
  if (!commands.push(command)) {
    Logger::GetLogger()->warn("Command queue full, dropping command");
  }
}

void GameBoy::processCommands() {
  Command command;
  while (commands.pop(command)) {
    std::visit(Overloaded{
                   [this](const SaveStateCommand&) { saveState(); },
                   [this](const ToggleFastForwardCommand&) {
                     framePacer->setFastForward(!framePacer->isFastForward());
                   },
               },
               command);
  }
}

void GameBoy::displayBootArt() {
  std::cout << R"(
    __ _  __ _ _ __ ___   ___| |__   ___  _   _
  / _` |/ _` | '_ ` _ \ / _ \ '_ \ / _ \| | | |
  | (_| | (_| | | | | | |  __/ |_) | (_) | |_| |
  \__, |\__,_|_| |_| |_|\___|_.__/ \___/ \__, |
    __/ |                                  __/ |
  |___/                                  |___/
  ⠀⠀⠀⠀⠀⠀⠀⢠⣤⣀⠀⠀⠀⠀⢀⣀⣤⣤⠀⠀⠀⠀⠀⠀⠀           ⢀⣠⣤⣤⣤⣤⣀⠀
  ⠀⠀⢀⢀⠀⠀⠀⢸⡿⠛⠛⠛⠛⠛⠉⠛⢿⣿⠀⠀⠀⠀⠀⠀⠀         ⣠⠞⠉⡟⡱⠔⠝⡆⠈⠳⣄
  ⠀⠠⣿⣿⣿⣄⠀⣼⠀⠀⠀⢂⣀⣀⡀⠀⠀⢹⡀⠀⠀⠀⠀⠀⠀ ⣀⣀⢰⠛⢦⠀⣰⠏⢀⡤⣓⣫⠭⣭⣗⠦⣄⢹⣆⠀⠀
  ⠀⢸⣿⣿⣿⣿⡷⠋⠈⠀⠀⠀⠀⠀⠀⠀⠈⠘⠣⡀⠀⠀⠀⠀⠀⠘⣇⠈⠛⣆⣈⣆⣿⣴⡫⢚⣒⣃⠀⣴⡔⡉⢪⣿⣿⠀⠀
  ⠀⠈⣿⣿⡿⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠘⣷⣦⡀⠀⠀⠀⢈⡗⢶⣯⡄⢸⠛⣿⣧⣈⣿⠟⠀⠛⢷⣡⣻⣿⠈⡧
  ⠀⠀⢹⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⣿⣿⣿⣦⠀  ⠈⢇⠤⠏⠀⠘⣦⣼⠁⢻⣧⣀⣀⣀⣼⠿⠁⣹⠶⠃⠀
  ⠀⠀⣸⣿⣿⣶⣶⣶⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣇   ⠀⠙⠒⠺⢶⡏⠉⢧⣀⠈⣯⢥⢽⠁⣀⣴⣇⡀⠀⠀
  ⠀⣤⡟⠛⠋⠉⠙⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠟⠉⠈⠋⠈⢿⣿⡿      ⠙⣆⢀⡟⠳⡮⣖⣦⢾⡟⠉⢆⠙⢦⡀
  ⢀⡉⠀⠀⣀⣤⣄⢈⣿⣿⣿⣿⣿⣿⣿⣿⣿⢀⣤⣤⣄⠀⠀⣴⡄      ⣿⣾⢰⣷⢻⡉⠉⠉⣷⣿⣾⡀⠀⡇
  ⠘⢇⠀⠰⣿⣿⢟⢼⣿⣿⣿⣿⣿⣿⣿⣿⡿⢜⠿⠿⠿⠀⡀⠀⠀
  ⠀⠀⠁⠀⠀⠀⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠀⠀⠈⠀
  )" << '\n';
}
