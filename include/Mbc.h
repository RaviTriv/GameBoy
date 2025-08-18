#pragma once

#include <cstdint>
#include <vector>

class MBC
{
public:
  virtual ~MBC() = default;
  virtual uint8_t read(uint16_t address) const = 0;
  virtual void write(uint16_t address, uint8_t value) = 0;
  MBC(std::vector<uint8_t> &rom) : romData(rom), ramData(empty) {};
  MBC(std::vector<uint8_t> &rom, std::vector<uint8_t> &ram,
      uint16_t romBanks, uint16_t ramBanks)
      : romData(rom), ramData(ram),
        romBanksCount(romBanks), ramBanksCount(ramBanks) {}

protected:
  uint16_t romBanksCount = 0;
  uint16_t ramBanksCount = 0;

  std::vector<uint8_t> &romData;
  std::vector<uint8_t> &ramData;

private:
  static std::vector<uint8_t> empty;
};

class MBC0 : public MBC
{
public:
  MBC0(std::vector<uint8_t> &rom)
      : MBC(rom) {}
  uint8_t read(uint16_t address) const override;
  void write(uint16_t address, uint8_t value) override;
};

// class MBC1 : public MBC
// {
// private:
//   bool ram_enabled = false;
//   uint8_t rom_bank = 1;
//   uint8_t ram_bank = 0;
//   uint8_t bankingMode = 0;

// public:
//   MBC1(std::vector<uint8_t> &rom, std::vector<uint8_t> &ram,
//        uint16_t romBanks, uint16_t ramBanks)
//       : MBC(rom, ram, romBanks, ramBanks) {}

//   uint8_t read(uint16_t address);
//   void write(uint16_t address, uint8_t value);
// };