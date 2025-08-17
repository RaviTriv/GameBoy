#include "../../include/Gameboy.h"
#include "../../include/Cartridge.h"
#include "../../include/Logger.h"

void GameBoy::init(std::string romPath)
{
  Logger::GetLogger()->info("Initializing GameBoy");
  Cartridge cartridge(romPath);
}