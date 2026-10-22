#include "Cartridge.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <ios>
#include <istream>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "EnumName.h"
#include "Logger.h"
#include "Mbc.h"
#include "Serializable.h"

Cartridge::Cartridge(std::string_view romPath)
    : Cartridge(readRomFile(romPath)) {}

Cartridge::Cartridge(std::vector<uint8_t> rom)
    : state(parseRom(std::move(rom))), mbc(createMbc()) {
  outputCartridgeInfo();
}

std::vector<uint8_t> Cartridge::readRomFile(std::string_view romPath) {
  std::ifstream cartridge(std::string(romPath),
                          std::ios::binary | std::ios::ate);

  if (!cartridge.is_open()) {
    throw std::runtime_error("Failed to open cartridge.");
  }

  const std::streamoff size = cartridge.tellg();
  if (size < 0) {
    throw std::runtime_error("Failed to read cartridge size.");
  }

  cartridge.seekg(0, std::ios::beg);

  std::vector<uint8_t> rom(static_cast<std::size_t>(size));

  if (!cartridge.read(reinterpret_cast<char*>(rom.data()),
                      static_cast<std::streamsize>(rom.size()))) {
    throw std::runtime_error("Failed to load cartridge data.");
  };

  return rom;
}

Cartridge::State Cartridge::parseRom(std::vector<uint8_t> romBytes) {
  State parsed{};
  parsed.romData = std::move(romBytes);
  parsed.romSize = parsed.romData.size();

  if (parsed.romSize < MIN_ROM_SIZE) {
    throw std::runtime_error("Cartridge is too small to contain a ROM header.");
  }

  if (parsed.romSize < 0x8000 || parsed.romSize > MAX_ROM_SIZE ||
      parsed.romSize % 0x4000 != 0) {
    std::ostringstream message;
    message << "Cartridge ROM size " << parsed.romSize
            << " bytes is invalid: must be 0x8000 to 0x800000 and a multiple "
               "of 0x4000.";
    throw std::runtime_error(message.str());
  }

  parsed.header = std::make_unique<RomHeader>();

  const uint8_t* rom = parsed.romData.data();

  std::copy_n(rom + ROM_HEADER_OFFSET, 4, parsed.header->entry.begin());

  std::copy_n(rom + ROM_HEADER_OFFSET + 0x04, 0x30,
              parsed.header->logo.begin());

  std::copy_n(rom + ROM_HEADER_OFFSET + 0x34, 16, parsed.header->title.begin());

  parsed.header->newLicenseeCode = static_cast<uint16_t>(
      (rom[ROM_HEADER_OFFSET + 0x44] << 8) | rom[ROM_HEADER_OFFSET + 0x45]);
  parsed.header->sgbFlag = rom[ROM_HEADER_OFFSET + 0x46];
  parsed.header->type =
      static_cast<CartridgeType>(rom[ROM_HEADER_OFFSET + 0x47]);
  parsed.header->romSize = rom[ROM_HEADER_OFFSET + 0x48];
  parsed.header->ramSize = rom[ROM_HEADER_OFFSET + 0x49];
  parsed.header->destCode = rom[ROM_HEADER_OFFSET + 0x4A];
  parsed.header->licenseeCode = rom[ROM_HEADER_OFFSET + 0x4B];
  parsed.header->version = rom[ROM_HEADER_OFFSET + 0x4C];
  parsed.header->checksum = rom[ROM_HEADER_OFFSET + 0x4D];
  parsed.header->globalChecksum = static_cast<uint16_t>(
      (rom[ROM_HEADER_OFFSET + 0x4E] << 8) | rom[ROM_HEADER_OFFSET + 0x4F]);

  uint8_t checksum = 0;
  for (std::size_t i = 0x134; i <= 0x14C; i++) {
    checksum = static_cast<uint8_t>(checksum - rom[i] - 1);
  }
  if (checksum != parsed.header->checksum) {
    Logger::GetLogger()->warn(
        "Header checksum mismatch: expected 0x{:02X}, computed 0x{:02X}",
        parsed.header->checksum, checksum);
  }

  int ramBanks = getRamBanksCount(parsed.romData.at(0x149));
  parsed.ramData.resize(static_cast<std::size_t>(ramBanks) * 0x2000);

  return parsed;
}

AnyMbc Cartridge::createMbc() {
  int romBanks = std::max(2, static_cast<int>(state.romSize / 0x4000));
  int ramBanks = getRamBanksCount(state.romData.at(0x149));

  switch (state.header->type) {
    case CartridgeType::ROM_ONLY:
    case CartridgeType::ROM_RAM:
    case CartridgeType::ROM_RAM_BATTERY:
      return AnyMbc(std::in_place_type<MBC0>, state.romData, state.ramData);
    case CartridgeType::MBC1:
    case CartridgeType::MBC1_RAM:
    case CartridgeType::MBC1_RAM_BATTERY:
      return AnyMbc(std::in_place_type<MBC1>, state.romData, state.ramData,
                    romBanks, ramBanks, isMbc1Multicart());
    case CartridgeType::MBC2:
    case CartridgeType::MBC2_BATTERY:
      return AnyMbc(std::in_place_type<MBC2>, state.romData, state.ramData,
                    romBanks, ramBanks);
    case CartridgeType::MBC3:
    case CartridgeType::MBC3_RAM:
    case CartridgeType::MBC3_RAM_BATTERY:
      return AnyMbc(std::in_place_type<MBC3>, state.romData, state.ramData,
                    romBanks, ramBanks, false);
    case CartridgeType::MBC3_TIMER_BATTERY:
    case CartridgeType::MBC3_TIMER_RAM_BATTERY:
      return AnyMbc(std::in_place_type<MBC3>, state.romData, state.ramData,
                    romBanks, ramBanks, true);
    case CartridgeType::MBC5:
    case CartridgeType::MBC5_RAM:
    case CartridgeType::MBC5_RAM_BATTERY:
      return AnyMbc(std::in_place_type<MBC5>, state.romData, state.ramData,
                    romBanks, ramBanks, false);
    case CartridgeType::MBC5_RUMBLE:
    case CartridgeType::MBC5_RUMBLE_RAM:
    case CartridgeType::MBC5_RUMBLE_RAM_BATTERY:
      return AnyMbc(std::in_place_type<MBC5>, state.romData, state.ramData,
                    romBanks, ramBanks, true);
    default: {
      std::ostringstream message;
      message << "Unsupported cartridge type: 0x" << std::hex
              << std::setfill('0') << std::setw(2)
              << static_cast<int>(state.header->type);
      throw std::runtime_error(message.str());
    }
  }
}

void Cartridge::outputCartridgeInfo() {
  Logger::GetLogger()->info("Cartridge Loaded");
  Logger::GetLogger()->info("Title: {}", state.header->title.data());
  Logger::GetLogger()->info("Cartridge Type: {}", enumName(state.header->type));
  Logger::GetLogger()->info("Rom Size: {} bytes", state.romSize);
  Logger::GetLogger()->info("Ram Size: {} bytes", state.ramData.size());
}

bool Cartridge::isMbc1Multicart() const {
  constexpr std::size_t MULTICART_SIZE = 0x100000;
  constexpr std::size_t GAME_SIZE = 0x40000;
  constexpr std::size_t LOGO_OFFSET = ROM_HEADER_OFFSET + 0x04;
  if (state.romSize != MULTICART_SIZE) {
    return false;
  }
  int logos = 0;
  for (std::size_t base = 0; base < MULTICART_SIZE; base += GAME_SIZE) {
    if (std::equal(state.header->logo.begin(), state.header->logo.end(),
                   state.romData.begin() +
                       static_cast<std::ptrdiff_t>(base + LOGO_OFFSET))) {
      logos++;
    }
  }
  return logos > 1;
}

uint8_t Cartridge::read(uint16_t address) const {
  return std::visit([address](const auto& chip) { return chip.read(address); },
                    mbc);
}

void Cartridge::write(uint16_t address, uint8_t value) {
  std::visit([address, value](auto& chip) { chip.write(address, value); }, mbc);
}

void Cartridge::tick(int cycles) {
  std::visit([cycles](auto& chip) { chip.tick(cycles); }, mbc);
}

int Cartridge::getRamBanksCount(uint8_t type) {
  switch (type) {
    case 0x00:
    case 0x01:
      return 0;
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
      Logger::GetLogger()->error("Unknown RAM size for cartridge type: {}",
                                 static_cast<int>(type));
      return 0;
  }
}

std::string Cartridge::getTitle() const {
  const auto& title = state.header->title;
  std::string result(title.begin(), std::ranges::find(title, '\0'));
  for (char& c : result) {
    if (c < 0x20 || c > 0x7E) {
      c = '_';
    }
  }
  return result;
}

std::span<const uint8_t, HEADER_LOGO_SIZE> Cartridge::getLogo() const {
  return state.header->logo;
}

bool Cartridge::hasBattery() const {
  switch (state.header->type) {
    case CartridgeType::MBC1_RAM_BATTERY:
    case CartridgeType::MBC2_BATTERY:
    case CartridgeType::ROM_RAM_BATTERY:
    case CartridgeType::MBC3_TIMER_BATTERY:
    case CartridgeType::MBC3_TIMER_RAM_BATTERY:
    case CartridgeType::MBC3_RAM_BATTERY:
    case CartridgeType::MBC5_RAM_BATTERY:
    case CartridgeType::MBC5_RUMBLE_RAM_BATTERY:
      return true;
    default:
      return false;
  }
}

void Cartridge::saveBattery(std::ostream& out) const {
  std::visit([&out](const auto& chip) { chip.saveBattery(out); }, mbc);
}

void Cartridge::loadBattery(std::istream& in) {
  std::visit([&in](auto& chip) { chip.loadBattery(in); }, mbc);
}

void Cartridge::save(std::ostream& out) const {
  serial::saveFields(out, *this);
  std::visit([&out](const auto& chip) { chip.save(out); }, mbc);
}

void Cartridge::load(std::istream& in) {
  serial::loadFields(in, *this);
  std::visit([&in](auto& chip) { chip.load(in); }, mbc);
}
