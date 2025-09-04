#include "../../../include/Cartridge.h"
#include "../../../include/Logger.h"

Cartridge::Cartridge(std::string_view romPath)
{
  loadCartridge(romPath);
}

void Cartridge::loadCartridge(std::string_view romPath)
{
  std::ifstream cartridge(romPath, std::ios::binary | std::ios::ate);

  if (!cartridge.is_open())
  {
    throw std::runtime_error("Failed to open cartridge.");
  }

  state.romSize = cartridge.tellg();

  cartridge.seekg(0, std::ios::beg);

  state.romData.resize(state.romSize);

  if (!cartridge.read(reinterpret_cast<char *>(state.romData.data()), state.romSize))
  {
    throw std::runtime_error("Failed to load cartridge data.");
  };

  state.header = std::make_unique<RomHeader>();

  const uint8_t *rom = state.romData.data();

  std::copy_n(rom + ROM_HEADER_OFFSET, 4, state.header->entry.begin());

  std::copy_n(rom + ROM_HEADER_OFFSET + 0x04, 0x30, state.header->logo.begin());

  std::copy_n(rom + ROM_HEADER_OFFSET + 0x34, 16, state.header->title.begin());

  state.header->newLicenseeCode = (rom[ROM_HEADER_OFFSET + 0x44] << 8) | rom[ROM_HEADER_OFFSET + 0x45];
  state.header->sgbFlag = rom[ROM_HEADER_OFFSET + 0x46];
  state.header->type = static_cast<CartridgeType>(rom[ROM_HEADER_OFFSET + 0x47]);
  state.header->romSize = rom[ROM_HEADER_OFFSET + 0x48];
  state.header->ramSize = rom[ROM_HEADER_OFFSET + 0x49];
  state.header->destCode = rom[ROM_HEADER_OFFSET + 0x4A];
  state.header->licenseeCode = rom[ROM_HEADER_OFFSET + 0x4B];
  state.header->version = rom[ROM_HEADER_OFFSET + 0x4C];
  state.header->checksum = rom[ROM_HEADER_OFFSET + 0x4D];
  state.header->globalChecksum = (rom[ROM_HEADER_OFFSET + 0x4E] << 8) | rom[ROM_HEADER_OFFSET + 0x4F];

  int romBanks = (state.romSize / 0x4000);
  int ramBanks = getRomBanksCount(state.romData.at(0x149));

  state.ramData.resize(ramBanks * 0x2000);

  switch (state.header->type)
  {
  case CartridgeType::ROM_ONLY:
    mbc = std::make_unique<MBC0>(state.romData);
    break;
  case CartridgeType::MBC1:
  case CartridgeType::MBC1_RAM:
  case CartridgeType::MBC1_RAM_BATTERY:
    mbc = std::make_unique<MBC1>(state.romData, state.ramData, romBanks, ramBanks);
    break;
  case CartridgeType::MBC2:
  case CartridgeType::MBC2_BATTERY:
    mbc = std::make_unique<MBC2>(state.romData, state.ramData, romBanks, ramBanks);
    break;
  case CartridgeType::MBC3:
  case CartridgeType::MBC3_RAM:
  case CartridgeType::MBC3_RAM_BATTERY:
  case CartridgeType::MBC3_TIMER_BATTERY:
  case CartridgeType::MBC3_TIMER_RAM_BATTERY:
    mbc = std::make_unique<MBC3>(state.romData, state.ramData, romBanks, ramBanks);
    break;
  default:
    Logger::GetLogger()->error("Unsupported cartridge type: {}", cartridgeType(state.header->type));
    break;
  }
  outputCartridgeInfo();
}

void Cartridge::outputCartridgeInfo()
{
  Logger::GetLogger()->info("Cartridge Loaded");
  Logger::GetLogger()->info("Title: {}", state.header->title.data());
  Logger::GetLogger()->info("Cartridge Type: {}", cartridgeType(state.header->type));
  Logger::GetLogger()->info("Rom Size: {} bytes", state.romSize);
  Logger::GetLogger()->info("Ram Size: {} bytes", state.ramData.size());
}

uint8_t Cartridge::read(uint16_t address) const
{
  return mbc->read(address);
}

void Cartridge::write(uint16_t address, uint8_t value)
{
  mbc->write(address, value);
}

int Cartridge::getRomBanksCount(uint8_t type) const
{
  switch (type)
  {
  case 0x00:
    return 0;
    break;
  case 0x01:
    return 0;
    break;
  case 0x02:
    return 1;
    break;
  case 0x03:
    return 4;
    break;
  case 0x04:
    return 16;
    break;
  case 0x05:
    return 8;
    break;
  default:
    Logger::GetLogger()->error("Unknown RAM size for cartridge type: {}", static_cast<int>(type));
  }
}

std::string Cartridge::cartridgeType(CartridgeType type)
{
  switch (type)
  {
  case CartridgeType::ROM_ONLY:
    return "ROM ONLY";
  case CartridgeType::MBC1:
    return "MBC1";
  case CartridgeType::MBC1_RAM:
    return "MBC1+RAM";
  case CartridgeType::MBC1_RAM_BATTERY:
    return "MBC1+RAM+BATTERY";
  case CartridgeType::MBC2:
    return "MBC2";
  case CartridgeType::MBC2_BATTERY:
    return "MBC2+BATTERY";
  case CartridgeType::MBC3_TIMER_BATTERY:
    return "MBC3+TIMER+BATTERY";
  case CartridgeType::MBC3_TIMER_RAM_BATTERY:
    return "MBC3+TIMER+RAM+BATTERY";
  case CartridgeType::MBC3:
    return "MBC3";
  case CartridgeType::MBC3_RAM:
    return "MBC3+RAM";
  case CartridgeType::MBC3_RAM_BATTERY:
    return "MBC3+RAM+BATTERY";
  default:
    return "UNKNOWN";
  }
}

std::string Cartridge::getTitle() const
{
  return std::string(state.header->title.data());
}