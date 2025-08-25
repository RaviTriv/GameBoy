#include "../../include/Gameboy.h"
#include "../../include/Bus.h"
#include "../../include/Cartridge.h"
#include "../../include/Cpu.h"
#include "../../include/Dma.h"
#include "../../include/Io.h"
#include "../../include/Lcd.h"
#include "../../include/Ppu.h"
#include "../../include/Ram.h"
#include "../../include/Timer.h"
#include "../../include/Ui.h"
#include "../../include/Logger.h"

void GameBoy::init(std::string romPath)
{
  Logger::GetLogger()->info("Initializing GameBoy");
  cartridge = std::make_unique<Cartridge>(romPath);
  dma = std::make_unique<DMA>(nullptr, nullptr);
  ram = std::make_unique<RAM>();
  io = std::make_unique<IO>(nullptr, nullptr, nullptr);
  lcd = std::make_shared<LCD>(dma);
  ppu = std::make_shared<PPU>(nullptr, lcd);
  ui = std::make_shared<UI>([this]()
                            { state.isRunning = false; });
  bus = std::make_unique<Bus>(cartridge, nullptr, dma, io, ppu, ram);
  cpu = std::make_shared<CPU>(
      [this](int cycles)
      { this->cycle(cycles); }, bus);
  timer = std::make_shared<Timer>(cpu);
  bus->setCpu(cpu);
  dma->setBus(bus);
  dma->setPpu(ppu);
  io->setTimer(timer);
  io->setCPU(cpu);
  io->setLCD(lcd);
  ppu->setCpu(cpu);
  state.isRunning = true;
}

void GameBoy::run()
{
  ui->init();
  cpuThread = std::thread(&GameBoy::cpuLoop, this);
  cpuThread.detach();

  // After Cpu is initialized
  timer->setDiv(0xABCC);

  while (state.isRunning)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    ui->handleEvents();
  }
}

void GameBoy::cpuLoop()
{
  while (state.isRunning)
  {
    if (!state.isPaused)
    {
      cpu->step();
    }
  }
}

void GameBoy::cycle(int cycles)
{
  for (int i = 0; i < cycles; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      state.ticks++;
      timer->tick();
      ppu->tick();
    }
    dma->tick();
  }
}
