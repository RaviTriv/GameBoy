#include "../../include/Gameboy.h"
#include "../../include/Bus.h"
#include "../../include/Cartridge.h"
#include "../../include/Cpu.h"
#include "../../include/Logger.h"

void GameBoy::init(std::string romPath)
{
  Logger::GetLogger()->info("Initializing GameBoy");
  cartridge = std::make_unique<Cartridge>(romPath);
  bus = std::make_unique<Bus>(cartridge);
  cpu = std::make_shared<CPU>(
      [this](int cycles)
      { this->cycle(cycles); }, bus);
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
