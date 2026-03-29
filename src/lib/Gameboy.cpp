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

GameBoy::GameBoy() = default;
GameBoy::~GameBoy() = default;

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
      [this](int cycles)
      { this->cycle(cycles); }, nullptr);
  ppu = std::make_unique<PPU>(*cpu);
  ui = std::make_unique<UI>(
      [this]()
      { state.isRunning = false; },
      [this]()
      { this->saveState(); },
      *ppu,
      *gamepad,
      *apu);
  timer = std::make_unique<Timer>(*cpu);
  io = std::make_unique<IO>(cpu->getInterruptRegs(), *timer, *lcd, *gamepad, *apu);
  bus = std::make_unique<Bus>(
      *cartridge, cpu->getInterruptRegs(),
      [this]()
      { return dma->isTransferring(); },
      *io, *ppu, *ram);
  cpu->setBus(bus.get());
  dma->setMemRead(*bus);
  ppu->setMemRead(*bus);
  ppu->setLcd(lcd.get());
  ppu->setGetTicks([this]()
                    { return ui->getTicks(); });
  ppu->setDelay([this](uint32_t ms)
                { ui->delay(ms); });
  stateSerializer = std::make_unique<StateSerializer>(*cpu, *ram, *ppu, *lcd);
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