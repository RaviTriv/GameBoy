#include "../../include/Gameboy.h"
#include "../../include/Bus.h"
#include "../../include/Cartridge.h"
#include "../../include/Cpu.h"
#include "../../include/Logger.h"

GameBoy::GameBoy()
{
  cpu = std::make_unique<CPU>();
}

void GameBoy::init(std::string romPath)
{
  Logger::GetLogger()->info("Initializing GameBoy");
  cartridge = std::make_unique<Cartridge>(romPath);
  bus = std::make_unique<Bus>(cartridge);
}