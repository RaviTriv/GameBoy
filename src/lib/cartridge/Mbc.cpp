#include "../../../include/Mbc.h"
#include "../../../include/Logger.h"

std::vector<uint8_t> MBC::empty;

uint8_t MBC0::read(uint16_t address) const
{
  return romData[address];
}

void MBC0::write(uint16_t address, uint8_t value)
{
  Logger::GetLogger()->error("Write not supported for MBC0");
}