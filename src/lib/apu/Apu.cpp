#include "../../../include/Apu.h"
#include "../../../include/Logger.h"

void APU::write(uint16_t address, uint8_t value)
{
  // TODO: Implement APU write
  Logger::GetLogger()->info("APU WRITE");
}

uint8_t APU::read(uint16_t address)
{
  // TODO: Implement APU read
  Logger::GetLogger()->info("APU READ");
  return 0;
}