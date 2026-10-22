// clang-format Language: Cpp
#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <variant>
#include <vector>

#include "Serializable.h"

class MBC {
 public:
  void saveBattery(std::ostream& out) const;
  void loadBattery(std::istream& in);
  void tick(int /*cycles*/) {}
  explicit MBC(std::vector<uint8_t>& rom) : romData(rom), ramData(empty) {}
  MBC(std::vector<uint8_t>& rom, std::vector<uint8_t>& ram, uint16_t romBanks,
      uint16_t ramBanks);

 protected:
  static constexpr std::size_t ROM_BANK_SIZE = 0x4000;
  static constexpr std::size_t RAM_BANK_SIZE = 0x2000;

  [[= serial::skip]] uint16_t romBanksCount = 0;
  [[= serial::skip]] uint16_t ramBanksCount = 0;

  [[= serial::skip]] std::vector<uint8_t>& romData;
  [[= serial::skip]] std::vector<uint8_t>& ramData;

 private:
  static std::vector<uint8_t> empty;
};

class MBC0 final : public MBC {
 public:
  MBC0(std::vector<uint8_t>& rom, std::vector<uint8_t>& ram)
      : MBC(rom, ram, static_cast<uint16_t>(rom.size() / ROM_BANK_SIZE),
            static_cast<uint16_t>(ram.size() / RAM_BANK_SIZE)) {}
  [[nodiscard]] uint8_t read(uint16_t address) const;
  void write(uint16_t address, uint8_t value);
  void save(std::ostream& out) const;
  void load(std::istream& in);
};

class MBC1 final : public MBC {
 private:
  bool ramEnabled = false;
  uint8_t romBank = 1;
  uint8_t ramBank = 0;
  uint8_t bankingMode = 0;
  [[= serial::skip]] bool multicart = false;

  [[nodiscard]] std::size_t upperBank() const;

 public:
  MBC1(std::vector<uint8_t>& rom, std::vector<uint8_t>& ram, uint16_t romBanks,
       uint16_t ramBanks, bool isMulticart)
      : MBC(rom, ram, romBanks, ramBanks), multicart(isMulticart) {}

  [[nodiscard]] uint8_t read(uint16_t address) const;
  void write(uint16_t address, uint8_t value);
  void save(std::ostream& out) const;
  void load(std::istream& in);
};

class MBC2 final : public MBC {
 private:
  bool ramEnabled = false;
  uint8_t romBank = 1;
  std::array<uint8_t, 512> ram_{};

 public:
  MBC2(std::vector<uint8_t>& rom, std::vector<uint8_t>& ram, uint16_t romBanks,
       uint16_t ramBanks)
      : MBC(rom, ram, romBanks, ramBanks) {}

  [[nodiscard]] uint8_t read(uint16_t address) const;
  void write(uint16_t address, uint8_t value);
  void saveBattery(std::ostream& out) const;
  void loadBattery(std::istream& in);
  void save(std::ostream& out) const;
  void load(std::istream& in);
};

class MBC3 final : public MBC {
 private:
  static constexpr std::size_t RTC_REGISTERS = 5;
  static constexpr uint32_t RTC_CYCLES_PER_SECOND = 1U << 20;
  using RtcRegisters = std::array<uint8_t, RTC_REGISTERS>;

  bool ramEnabled = false;
  [[= serial::skip]] bool hasRtc = false;
  uint8_t romBank = 1;
  uint8_t ramBank = 0;
  RtcRegisters rtc{};
  RtcRegisters latched{};
  bool latchArmed = false;
  uint32_t rtcCycles = 0;

  [[nodiscard]] std::size_t ramIndex(uint16_t address) const;
  [[nodiscard]] bool rtcHalted() const;
  void advanceSecond();
  void advanceSeconds(uint64_t seconds);

 public:
  MBC3(std::vector<uint8_t>& rom, std::vector<uint8_t>& ram, uint16_t romBanks,
       uint16_t ramBanks, bool withRtc)
      : MBC(rom, ram, romBanks, ramBanks), hasRtc(withRtc) {}

  [[nodiscard]] uint8_t read(uint16_t address) const;
  void write(uint16_t address, uint8_t value);
  void tick(int cycles);
  void saveBattery(std::ostream& out) const;
  void loadBattery(std::istream& in);
  void save(std::ostream& out) const;
  void load(std::istream& in);
};

class MBC5 final : public MBC {
 private:
  bool ramEnabled = false;
  [[= serial::skip]] bool rumble = false;
  uint16_t romBank = 1;
  uint8_t ramBank = 0;

  [[nodiscard]] std::size_t ramIndex(uint16_t address) const;

 public:
  MBC5(std::vector<uint8_t>& rom, std::vector<uint8_t>& ram, uint16_t romBanks,
       uint16_t ramBanks, bool hasRumble)
      : MBC(rom, ram, romBanks, ramBanks), rumble(hasRumble) {}

  [[nodiscard]] uint8_t read(uint16_t address) const;
  void write(uint16_t address, uint8_t value);
  void save(std::ostream& out) const;
  void load(std::istream& in);
};

template <typename M>
concept MbcChip =
    std::derived_from<M, MBC> &&
    requires(M chip, const M constChip, uint16_t address, uint8_t value,
             int cycles, std::ostream& out, std::istream& in) {
      { constChip.read(address) } -> std::same_as<uint8_t>;
      chip.write(address, value);
      chip.tick(cycles);
      constChip.saveBattery(out);
      chip.loadBattery(in);
      constChip.save(out);
      chip.load(in);
    };

static_assert(MbcChip<MBC0>);
static_assert(MbcChip<MBC1>);
static_assert(MbcChip<MBC2>);
static_assert(MbcChip<MBC3>);
static_assert(MbcChip<MBC5>);

using AnyMbc = std::variant<MBC0, MBC1, MBC2, MBC3, MBC5>;
