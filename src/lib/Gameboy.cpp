#include "Gameboy.h"

#include <spdlog/common.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <variant>

#include "Apu.h"
#include "Bus.h"
#include "Cartridge.h"
#include "Common.h"
#include "Cpu.h"
#include "Dma.h"
#include "FramePacer.h"
#include "Gamepad.h"
#include "Io.h"
#include "Lcd.h"
#include "Logger.h"
#include "Ppu.h"
#include "Ram.h"
#include "StateSerializer.h"
#include "Timer.h"
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
  cartridge = std::make_unique<Cartridge>(romPath);
  dma = std::make_unique<DMA>(
      [this](uint16_t addr, uint8_t val) { ppu->oamWrite(addr, val); });
  ram = std::make_unique<RAM>();
  gamepad = std::make_unique<Gamepad>();
  lcd = std::make_unique<LCD>([this](uint8_t value) { dma->start(value); });
  apu = std::make_unique<APU>();
  cpu = std::make_unique<CPU>(
      [](void* self, int cycles) {
        static_cast<GameBoy*>(self)->cycle(cycles);
      },
      this, nullptr);
  ppu = std::make_unique<PPU>(*cpu);
  ui =
      std::make_unique<UI>([this] { state.isRunning.store(false); },
                           [this] { pushCommand(SaveStateCommand{}); }, *ppu,
                           *gamepad, *apu);
  timer = std::make_unique<Timer>(*cpu);
  io = std::make_unique<IO>(cpu->getInterruptRegs(), *timer, *lcd, *gamepad,
                            *apu);
  bus = std::make_unique<Bus>(*cartridge, cpu->getInterruptRegs(), *dma, *io,
                              *ppu, *ram);
  cpu->setBus(bus.get());
  dma->setMemRead(*bus);
  ppu->setMemRead(*bus);
  ppu->setLcd(lcd.get());
  framePacer =
      std::make_unique<FramePacer>([this] { return ui->getTicks(); },
                                   [this](uint32_t ms) { ui->delay(ms); });
  framePacer->setFastForward(startFastForward);
  ui->setToggleFastForward(
      [this] { pushCommand(ToggleFastForwardCommand{}); });
  stateSerializer = std::make_unique<StateSerializer>(*cpu, *ram, *ppu, *lcd);
  state.isRunning.store(true);
  trace = enableTrace;
  loadSave = loadSaveOnStart;
  fastForward = startFastForward;
}

void GameBoy::saveState() {
  if (!stateSerializer->saveState(cartridge->getTitle())) {
    Logger::GetLogger()->error("Failed to save state for: {}",
                               cartridge->getTitle());
  }
}

void GameBoy::run() {
  ppu->init();
  ui->init();

  if (loadSave && (!stateSerializer->loadState(cartridge->getTitle()))) {
    Logger::GetLogger()->error("Failed to load save state for: {}",
                               cartridge->getTitle());
  }

  cpuThread = std::thread(&GameBoy::cpuLoop, this);

  uint32_t prevFrame = 0;
  while (state.isRunning.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(UI_THREAD_SLEEP_MS));
    ui->handleEvents();
    if (prevFrame != ppu->getCurrentFrame()) {
      ui->update();
    }
    prevFrame = ppu->getCurrentFrame();
  }

  cpuThread.join();
}

void GameBoy::cpuLoop() {
  while (state.isRunning.load()) {
    processCommands();
    cpu->step();
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

void GameBoy::cycle(int cycles) {
  for (int i = 0; i < cycles; i++) {
    for (int j = 0; j < CLOCK_CYCLES; j++) {
      state.ticks++;
      timer->tick();
      uint32_t prevFrame = ppu->getCurrentFrame();
      ppu->tick();
      if (prevFrame != ppu->getCurrentFrame()) {
        framePacer->onFrameComplete();
      }
      apu->tick();
    }
    dma->tick();
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
