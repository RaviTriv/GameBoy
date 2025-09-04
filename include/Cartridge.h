#pragma once

#include "./Mbc.h"

#include <array>
#include <cstdint>
#include <memory>
#include <fstream>
#include <vector>
#include <string>
#include <string_view>

enum class CartridgeType : uint8_t
{
  ROM_ONLY = 0x00,
  MBC1 = 0x01,
  MBC1_RAM = 0x02,
  MBC1_RAM_BATTERY = 0x03,
  MBC2 = 0x05,
  MBC2_BATTERY = 0x06,
  MBC3_TIMER_BATTERY = 0x0F,
  MBC3_TIMER_RAM_BATTERY = 0x10,
  MBC3 = 0x11,
  MBC3_RAM = 0x12,
  MBC3_RAM_BATTERY = 0x13,
};

struct RomHeader
{
  std::array<uint8_t, 4> entry;
  std::array<uint8_t, 0x30> logo;
  std::array<char, 16> title;

  uint16_t newLicenseeCode;
  uint8_t sgbFlag;
  CartridgeType type;
  uint8_t romSize;
  uint8_t ramSize;
  uint8_t destCode;
  uint8_t licenseeCode;
  uint8_t version;
  uint8_t checksum;
  uint16_t globalChecksum;
};

class MBC;
class Cartridge
{
public:
  struct State
  {
    std::array<char, 1024> filename;
    std::size_t romSize;
    std::vector<uint8_t> romData;
    std::vector<uint8_t> ramData;
    std::unique_ptr<RomHeader> header;
  };
  Cartridge(std::string_view romPath);
  uint8_t read(uint16_t address) const;
  void write(uint16_t address, uint8_t value);
  std::string getTitle() const;

private:
  State state;
  std::unique_ptr<MBC> mbc;
  static constexpr size_t ROM_HEADER_OFFSET = 0x100;

  void loadCartridge(std::string_view romPath);

  int getRomBanksCount(uint8_t type) const;
  static std::string cartridgeType(CartridgeType type);

  void outputCartridgeInfo();
};