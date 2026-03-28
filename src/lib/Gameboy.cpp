#include "../../include/Gameboy.h"
#include "../../include/Gamepad.h"
#include "../../include/Apu.h"
#include "../../include/Bus.h"
#include "../../include/Cartridge.h"
#include "../../include/Common.h"
#include "../../include/Cpu.h"
#include "../../include/Dma.h"
#include "../../include/Io.h"
#include "../../include/Lcd.h"
#include "../../include/Ppu.h"
#include "../../include/Ram.h"
#include "../../include/StateSerializer.h"
#include "../../include/Timer.h"
#include "../../include/Ui.h"
#include "../../include/Logger.h"

#include <iostream>

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
  io = std::make_unique<IO>(nullptr, nullptr, nullptr, gamepad, nullptr);
  lcd = std::make_shared<LCD>(
      [this](uint8_t value)
      { dma->start(value); });
  ppu = std::make_shared<PPU>(nullptr, nullptr, lcd, nullptr);
  ui = std::make_shared<UI>(
      [this]()
      { state.isRunning = false; },
      [this]()
      { this->saveState(); },
      ppu,
      gamepad,
      nullptr);
  apu = std::make_shared<APU>();
  bus = std::make_unique<Bus>(cartridge, nullptr, dma, io, ppu, ram);
  cpu = std::make_shared<CPU>(
      [this](int cycles)
      { this->cycle(cycles); }, bus);
  timer = std::make_shared<Timer>(*cpu);
  bus->setCpu(cpu);
  dma->setMemRead(*bus);
  io->setTimer(timer);
  io->setCPU(cpu);
  io->setLCD(lcd);
  io->setGamepad(gamepad);
  ppu->setCpu(cpu);
  ppu->setBus(bus);
  ui->setPpu(ppu);
  ppu->setUi(ui);

  io->setApu(apu);
  ui->setApu(apu);
  stateSerializer = std::make_shared<StateSerializer>(cpu, ram, ppu, lcd);
  state.isRunning = true;
  this->trace = trace;
  this->loadSave = loadSave;
  this->fastForward = fastForward;
  ppu->setFastForward(fastForward);
}

void GameBoy::saveState()
{
  stateSerializer->saveState(cartridge->getTitle());
}

void GameBoy::run()
{
  ppu->init();
  ui->init();

  if (loadSave)
  {
    stateSerializer->loadState(cartridge->getTitle());
  }

  cpuThread = std::thread(&GameBoy::cpuLoop, this);
  cpuThread.detach();

  uint32_t prevFrame = 0;
  while (state.isRunning)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(UI_THREAD_SLEEP_MS));
    ui->handleEvents();
    if (prevFrame != ppu->getCurrentFrame())
    {
      ui->update();
    }
    prevFrame = ppu->getCurrentFrame();
  }
}

void GameBoy::cpuLoop()
{
  while (state.isRunning)
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
      ppu->tick();
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