#include "Gameboy.h"
#include "Gamepad.h"
#include "Apu.h"
#include "Bus.h"
#include "Cartridge.h"
#include "Common.h"
#include "Cpu.h"
#include "Dma.h"
#include "FramePacer.h"
#include "Io.h"
#include "Lcd.h"
#include "Ppu.h"
#include "Ram.h"
#include "StateSerializer.h"
#include "Timer.h"
#include "Ui.h"
#include "Logger.h"

#include <iostream>

GameBoy::GameBoy() = default;
GameBoy::~GameBoy()
{
  state.isRunning.store(false);
  if (cpuThread.joinable())
  {
    cpuThread.join();
  }
}

void GameBoy::init(std::string romPath, bool trace, bool loadSave, bool fastForward)
{
  Logger::GetLogger()->set_level(spdlog::level::off);
  if (trace)
  {
    Logger::GetLogger()->set_level(spdlog::level::trace);
  }
  displayBootArt();
  Logger::GetLogger()->info("Initializing GameBoy");
  cartridge = std::make_unique<Cartridge>(romPath);
  dma = std::make_unique<DMA>(
      [this](uint16_t addr, uint8_t val)
      { ppu->oamWrite(addr, val); });
  ram = std::make_unique<RAM>();
  gamepad = std::make_unique<Gamepad>();
  lcd = std::make_unique<LCD>(
      [this](uint8_t value)
      { dma->start(value); });
  apu = std::make_unique<APU>();
  cpu = std::make_unique<CPU>(
      [](void *self, int cycles)
      { static_cast<GameBoy *>(self)->cycle(cycles); }, this, nullptr);
  ppu = std::make_unique<PPU>(*cpu);
  ui = std::make_unique<UI>(
      [this]()
      { state.isRunning.store(false); },
      [this]()
      { this->saveState(); },
      *ppu,
      *gamepad,
      *apu);
  timer = std::make_unique<Timer>(*cpu);
  io = std::make_unique<IO>(cpu->getInterruptRegs(), *timer, *lcd, *gamepad, *apu);
  bus = std::make_unique<Bus>(
      *cartridge, cpu->getInterruptRegs(),
      *dma,
      *io, *ppu, *ram);
  cpu->setBus(bus.get());
  dma->setMemRead(*bus);
  ppu->setMemRead(*bus);
  ppu->setLcd(lcd.get());
  framePacer = std::make_unique<FramePacer>(
      [this]() { return ui->getTicks(); },
      [this](uint32_t ms) { ui->delay(ms); });
  framePacer->setFastForward(fastForward);
  ui->setToggleFastForward([this]()
                           { framePacer->setFastForward(!framePacer->isFastForward()); });
  stateSerializer = std::make_unique<StateSerializer>(*cpu, *ram, *ppu, *lcd);
  state.isRunning.store(true);
  this->trace = trace;
  this->loadSave = loadSave;
  this->fastForward = fastForward;
}

void GameBoy::saveState()
{
  if (!stateSerializer->saveState(cartridge->getTitle()))
  {
    Logger::GetLogger()->error("Failed to save state for: {}", cartridge->getTitle());
  }
}

void GameBoy::run()
{
  ppu->init();
  ui->init();

  if (loadSave)
  {
    if (!stateSerializer->loadState(cartridge->getTitle()))
    {
      Logger::GetLogger()->error("Failed to load save state for: {}", cartridge->getTitle());
    }
  }

  cpuThread = std::thread(&GameBoy::cpuLoop, this);

  uint32_t prevFrame = 0;
  while (state.isRunning.load())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(UI_THREAD_SLEEP_MS));
    ui->handleEvents();
    if (prevFrame != ppu->getCurrentFrame())
    {
      ui->update();
    }
    prevFrame = ppu->getCurrentFrame();
  }

  cpuThread.join();
}

void GameBoy::cpuLoop()
{
  while (state.isRunning.load())
  {
    cpu->step();
  }
}

void GameBoy::cycle(int cycles)
{
  for (int i = 0; i < cycles; i++)
  {
    for (int j = 0; j < CLOCK_CYCLES; j++)
    {
      state.ticks++;
      timer->tick();
      uint32_t prevFrame = ppu->getCurrentFrame();
      ppu->tick();
      if (prevFrame != ppu->getCurrentFrame())
      {
        framePacer->onFrameComplete();
      }
      apu->tick();
    }
    dma->tick();
  }
}

void GameBoy::displayBootArt()
{
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
  )" << std::endl;
}
