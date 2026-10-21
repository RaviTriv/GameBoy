// clang-format Language: Cpp
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "Serializable.h"

class MBC {
 public:
  virtual ~MBC() = default;
  virtual void save(std::ostream& out) const = 0;
  virtual void load(std::istream& in) = 0;
  [[nodiscard]] virtual uint8_t read(uint16_t address) const = 0;
  virtual void write(uint16_t address, uint8_t value) = 0;
  virtual void saveBattery(std::ostream& out) const;
  virtual void loadBattery(std::istream& in);
  virtual void tick(int /*cycles*/) {}
  explicit MBC(std::vector<uint8_t>& rom) : romData(rom), ramData(empty) {}
  MBC(std::vector<uint8_t>& rom, std::vector<uint8_t>& ram, uint16_t romBanks,
      uint16_t ramBanks)
      : romBanksCount(romBanks),
        ramBanksCount(ramBanks),
        romData(rom),
        ramData(ram) {}

 protected:
  [[= serial::skip]] uint16_t romBanksCount = 0;
  [[= serial::skip]] uint16_t ramBanksCount = 0;

  [[= serial::skip]] std::vector<uint8_t>& romData;
  [[= serial::skip]] std::vector<uint8_t>& ramData;

 private:
  static std::vector<uint8_t> empty;
};

class MBC0 : public MBC {
 public:
  MBC0(std::vector<uint8_t>& rom, std::vector<uint8_t>& ram)
      : MBC(rom, ram, 0, 0) {}
  [[nodiscard]] uint8_t read(uint16_t address) const override;
  void write(uint16_t address, uint8_t value) override;
  void save(std::ostream& out) const override;
  void load(std::istream& in) override;
};

class MBC1 : public MBC {
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

  [[nodiscard]] uint8_t read(uint16_t address) const override;
  void write(uint16_t address, uint8_t value) override;
  void save(std::ostream& out) const override;
  void load(std::istream& in) override;
};

class MBC2 : public MBC {
 private:
  bool ramEnabled = false;
  uint8_t romBank = 1;
  std::array<uint8_t, 512> ram_{};

 public:
  MBC2(std::vector<uint8_t>& rom, std::vector<uint8_t>& ram, uint16_t romBanks,
       uint16_t ramBanks)
      : MBC(rom, ram, romBanks, ramBanks) {}

  [[nodiscard]] uint8_t read(uint16_t address) const override;
  void write(uint16_t address, uint8_t value) override;
  void saveBattery(std::ostream& out) const override;
  void loadBattery(std::istream& in) override;
  void save(std::ostream& out) const override;
  void load(std::istream& in) override;
};

class MBC3 : public MBC {
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

  [[nodiscard]] uint8_t read(uint16_t address) const override;
  void write(uint16_t address, uint8_t value) override;
  void tick(int cycles) override;
  void saveBattery(std::ostream& out) const override;
  void loadBattery(std::istream& in) override;
  void save(std::ostream& out) const override;
  void load(std::istream& in) override;
};

class MBC5 : public MBC {
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

  [[nodiscard]] uint8_t read(uint16_t address) const override;
  void write(uint16_t address, uint8_t value) override;
  void save(std::ostream& out) const override;
  void load(std::istream& in) override;
};
