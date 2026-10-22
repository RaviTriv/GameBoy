#pragma once

#include <array>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "./EnumName.h"
#include "./Mbc.h"
#include "./Serializable.h"

enum class CartridgeType : uint8_t {
  ROM_ONLY[[= EnumLabel("ROM ONLY")]] = 0x00,
  MBC1 = 0x01,
  MBC1_RAM[[= EnumLabel("MBC1+RAM")]] = 0x02,
  MBC1_RAM_BATTERY[[= EnumLabel("MBC1+RAM+BATTERY")]] = 0x03,
  MBC2 = 0x05,
  MBC2_BATTERY[[= EnumLabel("MBC2+BATTERY")]] = 0x06,
  ROM_RAM[[= EnumLabel("ROM+RAM")]] = 0x08,
  ROM_RAM_BATTERY[[= EnumLabel("ROM+RAM+BATTERY")]] = 0x09,
  MBC3_TIMER_BATTERY[[= EnumLabel("MBC3+TIMER+BATTERY")]] = 0x0F,
  MBC3_TIMER_RAM_BATTERY[[= EnumLabel("MBC3+TIMER+RAM+BATTERY")]] = 0x10,
  MBC3 = 0x11,
  MBC3_RAM[[= EnumLabel("MBC3+RAM")]] = 0x12,
  MBC3_RAM_BATTERY[[= EnumLabel("MBC3+RAM+BATTERY")]] = 0x13,
  MBC5 = 0x19,
  MBC5_RAM[[= EnumLabel("MBC5+RAM")]] = 0x1A,
  MBC5_RAM_BATTERY[[= EnumLabel("MBC5+RAM+BATTERY")]] = 0x1B,
  MBC5_RUMBLE[[= EnumLabel("MBC5+RUMBLE")]] = 0x1C,
  MBC5_RUMBLE_RAM[[= EnumLabel("MBC5+RUMBLE+RAM")]] = 0x1D,
  MBC5_RUMBLE_RAM_BATTERY[[= EnumLabel("MBC5+RUMBLE+RAM+BATTERY")]] = 0x1E,
};

struct RomHeader {
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
class Cartridge {
 public:
  struct State {
    [[= serial::skip]] std::array<char, 1024> filename;
    [[= serial::skip]] std::size_t romSize;
    [[= serial::skip]] std::vector<uint8_t> romData;
    std::vector<uint8_t> ramData;
    [[= serial::skip]] std::unique_ptr<RomHeader> header;
  };
  explicit Cartridge(std::string_view romPath);
  explicit Cartridge(std::vector<uint8_t> rom);
  Cartridge(const Cartridge&) = delete ("the MBC holds references into state");
  Cartridge& operator=(const Cartridge&) = delete ("the MBC holds references into state");
  Cartridge(Cartridge&&) = delete ("the MBC holds references into state");
  Cartridge& operator=(Cartridge&&) = delete ("the MBC holds references into state");
  ~Cartridge() = default;
  [[nodiscard]] uint8_t read(uint16_t address) const;
  void write(uint16_t address, uint8_t value);
  void tick(int cycles);
  [[nodiscard]] std::string getTitle() const;
  [[nodiscard]] bool hasBattery() const;
  void saveBattery(std::ostream& out) const;
  void loadBattery(std::istream& in);
  void save(std::ostream& out) const;
  void load(std::istream& in);

 private:
  State state;
  [[= serial::skip]] AnyMbc mbc;
  static constexpr size_t ROM_HEADER_OFFSET = 0x100;

  [[nodiscard]] static std::vector<uint8_t> readRomFile(std::string_view romPath);
  [[nodiscard]] static State parseRom(std::vector<uint8_t> rom);
  [[nodiscard]] AnyMbc createMbc();
  static constexpr std::size_t MIN_ROM_SIZE = 0x150;
  static constexpr std::size_t MAX_ROM_SIZE = 0x800000;

  [[nodiscard]] static int getRamBanksCount(uint8_t type);
  [[nodiscard]] bool isMbc1Multicart() const;

  void outputCartridgeInfo();
};