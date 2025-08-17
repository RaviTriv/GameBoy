#include "../../include/Cartridge.h"
#include "../../include/Logger.h"

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

  cartridgeState.romSize = cartridge.tellg();

  cartridge.seekg(0, std::ios::beg);

  cartridgeState.romData.resize(cartridgeState.romSize);

  if (!cartridge.read(reinterpret_cast<char *>(cartridgeState.romData.data()), cartridgeState.romSize))
  {
    throw std::runtime_error("Failed to load cartridge data.");
  };

  cartridgeState.header = std::make_unique<RomHeader>();

  const uint8_t *rom = cartridgeState.romData.data();

  std::copy_n(rom + ROM_HEADER_OFFSET, 4, cartridgeState.header->entry.begin());

  std::copy_n(rom + ROM_HEADER_OFFSET + 0x04, 0x30, cartridgeState.header->logo.begin());

  std::copy_n(rom + ROM_HEADER_OFFSET + 0x34, 16, cartridgeState.header->title.begin());

  cartridgeState.header->newLicenseeCode = (rom[ROM_HEADER_OFFSET + 0x44] << 8) | rom[ROM_HEADER_OFFSET + 0x45];
  cartridgeState.header->sgbFlag = rom[ROM_HEADER_OFFSET + 0x46];
  cartridgeState.header->type = static_cast<CartridgeType>(rom[ROM_HEADER_OFFSET + 0x47]);
  cartridgeState.header->romSize = rom[ROM_HEADER_OFFSET + 0x48];
  cartridgeState.header->ramSize = rom[ROM_HEADER_OFFSET + 0x49];
  cartridgeState.header->destCode = rom[ROM_HEADER_OFFSET + 0x4A];
  cartridgeState.header->licenseeCode = rom[ROM_HEADER_OFFSET + 0x4B];
  cartridgeState.header->version = rom[ROM_HEADER_OFFSET + 0x4C];
  cartridgeState.header->checksum = rom[ROM_HEADER_OFFSET + 0x4D];
  cartridgeState.header->globalChecksum = (rom[ROM_HEADER_OFFSET + 0x4E] << 8) | rom[ROM_HEADER_OFFSET + 0x4F];

  Logger::GetLogger()
      ->info("Cartridge Loaded | Title: {}, Version: {}, Type: {}, Size: {}", cartridgeState.header->title.data(), cartridgeState.header->version, cartridgeType(cartridgeState.header->type), cartridgeState.romSize);
}

uint8_t Cartridge::read(uint16_t address) const
{
  return cartridgeState.romData[address];
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