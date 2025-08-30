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

uint8_t MBC1::read(uint16_t address) const
{
  if (address < 0x4000)
  {
    int bank = bankingMode * (ramBank << 5) % romBanksCount;
    return romData[bank * 0x4000 + address];
  }
  else if (address < 0x8000)
  {
    int bank = ((ramBank << 5) | romBank) % romBanksCount;
    return romData[bank * 0x4000 + address - 0x4000];
  }
  else if (address >= 0xA000 && address < 0xC000)
  {
    if (ramEnabled)
    {
      int bank = bankingMode * ramBank % ramBanksCount;
      return ramData[bank * 0x2000 + address - 0xA000];
    }
  }
  return 0xFF;
}

void MBC1::write(uint16_t address, uint8_t value)
{
  if (address < 0x2000)
  {
    ramEnabled = (value & 0x0F) == 0x0A;
  }
  else if (address < 0x4000)
  {
    value &= 0x1F;
    if (value == 0)
    {
      value = 0x01;
    }
    romBank = value;
  }
  else if (address < 0x6000)
  {
    ramBank = value & 0x03;
  }
  else if (address < 0x8000)
  {
    bankingMode = value & 0x01;
  }
  else if (address >= 0xA000 && address < 0xC000)
  {
    if (ramEnabled)
    {
      int bank = (bankingMode * ramBank) % ramBanksCount;
      ramData[bank * 0x2000 + address - 0xA000] = value;
    }
  }
}

uint8_t MBC2::read(uint16_t address) const
{
  if (address < 0x4000)
  {
    return romData[address];
  }
  else if (address < 0x8000)
  {
    return romData[romBank * 0x4000 + address - 0x4000];
  }
  else if (address >= 0xA000 && address < 0xC000)
  {
    if (ramEnabled)
    {
      return ramData[ramBank * 0x2000 + address - 0xA000];
    }
  }
  return 0;
}

void MBC2::write(uint16_t address, uint8_t value)
{
  if (address < 0x2000)
  {
    if ((address & 0x0100) == 0)
    {
      ramEnabled = value == 0x0a;
    }
  }
  else if (address < 0x4000)
  {
    if ((address & 0x0100) != 0)
    {
      romBank = value;
    }
  }
  else if (address >= 0xA000 && address < 0xC000)
  {
    if (ramEnabled)
    {
      ramData[ramBank * 0x2000 + address - 0xA000] = value;
    }
  }
}

uint8_t MBC3::read(uint16_t address) const
{
  if (address < 0x4000)
  {
    return romData[address];
  }
  else if (address < 0x8000)
  {
    return romData[romBank * 0x4000 + address - 0x4000];
  }
  else if (address >= 0xA000 && address < 0xC000)
  {
    if (ramEnabled)
    {
      if (ramBank <= 0x03)
      {
        return ramData[ramBank * 0x2000 + address - 0xA000];
      }
    }
  }
  return 0;
}

void MBC3::write(uint16_t address, uint8_t value)
{
  if (address < 0x2000)
  {
    ramEnabled = (value & 0x0f) == 0x0a;
  }
  else if (address < 0x4000)
  {
    romBank = value & 0x7f;
    if (romBank == 0x00)
    {
      romBank = 0x01;
    }
  }
  else if (address < 0x6000)
  {
    ramBank = value & 0x0f;
  }
  else if (address >= 0xA000 && address < 0xC000)
  {
    if (ramEnabled)
    {
      if (ramBank <= 0x03)
      {
        ramData[ramBank * 0x2000 + address - 0xA000] = value;
      }
    }
  }
}
