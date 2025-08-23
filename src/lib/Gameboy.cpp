#include "../../include/Gameboy.h"
#include "../../include/Bus.h"
#include "../../include/Cartridge.h"
#include "../../include/Cpu.h"
#include "../../include/Io.h"
#include "../../include/Ram.h"
#include "../../include/Ui.h"
#include "../../include/Logger.h"

void GameBoy::init(std::string romPath)
{
  Logger::GetLogger()->info("Initializing GameBoy");
  cartridge = std::make_unique<Cartridge>(romPath);
  ram = std::make_unique<RAM>();
  io = std::make_unique<IO>();
  bus = std::make_unique<Bus>(cartridge, io, ram);
  cpu = std::make_shared<CPU>(
      [this](int cycles)
      { this->cycle(cycles); }, bus);
  state.isRunning = true;
  ui = std::make_shared<UI>();
}

void GameBoy::run()
{
  ui->init();
  while (state.isRunning)
  {
    cpu->step();
  }
}

void GameBoy::cycle(int cycles)
{
  for (int i = 0; i < cycles; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      state.ticks++;
    }
  }
}
