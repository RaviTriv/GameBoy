#include "Mbc.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <istream>
#include <ostream>
#include <vector>

#include "Serializable.h"

std::vector<uint8_t> MBC::empty;

MBC::MBC(std::vector<uint8_t>& rom, std::vector<uint8_t>& ram,
         uint16_t romBanks, uint16_t ramBanks)
    : romBanksCount(romBanks),
      ramBanksCount(ramBanks),
      romData(rom),
      ramData(ram) {
  contract_assert(romBanks >= 2 && rom.size() == romBanks * ROM_BANK_SIZE);
  contract_assert(ram.size() == ramBanks * RAM_BANK_SIZE);
}

void MBC::saveBattery(std::ostream& out) const {
  out.write(reinterpret_cast<const char*>(ramData.data()),
            static_cast<std::streamsize>(ramData.size()));
}

void MBC::loadBattery(std::istream& in) {
  in.read(reinterpret_cast<char*>(ramData.data()),
          static_cast<std::streamsize>(ramData.size()));
}

uint8_t MBC0::read(uint16_t address) const {
  if (address < 0x8000) {
    return romData[address];
  }
  if (address >= 0xA000 && address < 0xC000 && !ramData.empty()) {
    return ramData[(address - 0xA000) % ramData.size()];
  }
  return 0xFF;
}

void MBC0::write(uint16_t address, uint8_t value) {
  if (address >= 0xA000 && address < 0xC000 && !ramData.empty()) {
    ramData[(address - 0xA000) % ramData.size()] = value;
  }
}

std::size_t MBC1::upperBank() const {
  return static_cast<std::size_t>(ramBank) << (multicart ? 4 : 5);
}

uint8_t MBC1::read(uint16_t address) const {
  if (address < 0x4000) {
    auto bank = (bankingMode * upperBank()) % romBanksCount;
    return romData[bank * 0x4000 + address];
  }
  if (address < 0x8000) {
    const auto lower =
        static_cast<std::size_t>(romBank & (multicart ? 0x0F : 0x1F));
    auto bank = (upperBank() | lower) % romBanksCount;
    return romData[bank * 0x4000 + address - 0x4000];
  }
  if ((address >= 0xA000 && address < 0xC000) &&
      (ramEnabled && !ramData.empty())) {
    auto bank = static_cast<std::size_t>(bankingMode * ramBank % ramBanksCount);
    return ramData[bank * 0x2000 + address - 0xA000];
  }

  return 0xFF;
}

void MBC1::write(uint16_t address, uint8_t value) {
  if (address < 0x2000) {
    ramEnabled = (value & 0x0F) == 0x0A;
  } else if (address < 0x4000) {
    value &= 0x1F;
    if (value == 0) {
      value = 0x01;
    }
    romBank = value;
  } else if (address < 0x6000) {
    ramBank = value & 0x03;
  } else if (address < 0x8000) {
    bankingMode = value & 0x01;
  } else if ((address >= 0xA000 && address < 0xC000) &&
             (ramEnabled && !ramData.empty())) {
    auto bank =
        static_cast<std::size_t>((bankingMode * ramBank) % ramBanksCount);
    ramData[bank * 0x2000 + address - 0xA000] = value;
  }
}

uint8_t MBC2::read(uint16_t address) const {
  if (address < 0x4000) {
    return romData[address];
  }
  if (address < 0x8000) {
    contract_assert(romBank < romBanksCount);
    return romData[static_cast<std::size_t>(romBank) * 0x4000 + address -
                   0x4000];
  }
  if (address >= 0xA000 && address < 0xC000) {
    if (!ramEnabled) {
      return 0xFF;
    }
    return 0xF0 | ram_[address & 0x1FF];
  }
  return 0xFF;
}

void MBC2::write(uint16_t address, uint8_t value) {
  if (address < 0x4000) {
    if ((address & 0x0100) == 0) {
      ramEnabled = (value & 0x0F) == 0x0A;
    } else {
      romBank = value & 0x0F;
      if (romBank == 0) {
        romBank = 1;
      }
      romBank = static_cast<uint8_t>(romBank % romBanksCount);
    }
  } else if ((address >= 0xA000 && address < 0xC000) && ramEnabled) {
    ram_[address & 0x1FF] = value & 0x0F;
  }
}

void MBC2::saveBattery(std::ostream& out) const {
  out.write(reinterpret_cast<const char*>(ram_.data()),
            static_cast<std::streamsize>(ram_.size()));
}

void MBC2::loadBattery(std::istream& in) {
  in.read(reinterpret_cast<char*>(ram_.data()),
          static_cast<std::streamsize>(ram_.size()));
  for (auto& nibble : ram_) {
    nibble &= 0x0F;
  }
}

std::size_t MBC3::ramIndex(uint16_t address) const {
  const auto bank = static_cast<std::size_t>(ramBank % ramBanksCount);
  return bank * 0x2000 + address - 0xA000;
}

bool MBC3::rtcHalted() const { return (rtc[4] & 0x40) != 0; }

void MBC3::advanceSecond() {
  const auto step = [](uint8_t& value, uint8_t mask, uint8_t limit) {
    value = static_cast<uint8_t>((value + 1) & mask);
    if (value != limit) {
      return false;
    }
    value = 0;
    return true;
  };
  if (!step(rtc[0], 0x3F, 60) || !step(rtc[1], 0x3F, 60) ||
      !step(rtc[2], 0x1F, 24)) {
    return;
  }
  if (++rtc[3] != 0) {
    return;
  }
  if ((rtc[4] & 0x01) != 0) {
    rtc[4] = static_cast<uint8_t>((rtc[4] & 0xFE) | 0x80);
  } else {
    rtc[4] |= 0x01;
  }
}

void MBC3::advanceSeconds(uint64_t seconds) {
  constexpr uint64_t SECONDS_PER_DAY = 86400;
  while (seconds > 0 && (rtc[0] != 0 || rtc[1] != 0 || rtc[2] != 0)) {
    advanceSecond();
    seconds--;
  }
  for (; seconds >= SECONDS_PER_DAY; seconds -= SECONDS_PER_DAY) {
    if (++rtc[3] == 0) {
      if ((rtc[4] & 0x01) != 0) {
        rtc[4] = static_cast<uint8_t>((rtc[4] & 0xFE) | 0x80);
      } else {
        rtc[4] |= 0x01;
      }
    }
  }
  for (; seconds > 0; seconds--) {
    advanceSecond();
  }
}

void MBC3::tick(int cycles) {
  if (!hasRtc || rtcHalted()) {
    return;
  }
  rtcCycles += static_cast<uint32_t>(cycles);
  while (rtcCycles >= RTC_CYCLES_PER_SECOND) {
    rtcCycles -= RTC_CYCLES_PER_SECOND;
    advanceSecond();
  }
}

uint8_t MBC3::read(uint16_t address) const {
  if (address < 0x4000) {
    return romData[address];
  }
  if (address < 0x8000) {
    contract_assert(romBank < romBanksCount);
    return romData[static_cast<std::size_t>(romBank) * 0x4000 + address -
                   0x4000];
  }
  if (address < 0xA000 || address >= 0xC000 || !ramEnabled) {
    return 0xFF;
  }
  if (ramBank <= 0x07 && !ramData.empty()) {
    return ramData[ramIndex(address)];
  }
  if (hasRtc && ramBank >= 0x08 && ramBank <= 0x0C) {
    return latched[ramBank - 0x08U];
  }
  return 0xFF;
}

void MBC3::write(uint16_t address, uint8_t value) {
  constexpr RtcRegisters RTC_MASKS = {0x3F, 0x3F, 0x1F, 0xFF, 0xC1};
  if (address < 0x2000) {
    ramEnabled = (value & 0x0f) == 0x0a;
  } else if (address < 0x4000) {
    romBank = value;
    if (romBank == 0x00) {
      romBank = 0x01;
    }
    romBank = static_cast<uint8_t>(romBank % romBanksCount);
  } else if (address < 0x6000) {
    ramBank = value & 0x0f;
  } else if (address < 0x8000) {
    if (latchArmed && value == 0x01) {
      latched = rtc;
    }
    latchArmed = value == 0x00;
  } else if (address < 0xA000 || address >= 0xC000 || !ramEnabled) {
    return;
  } else if (ramBank <= 0x07 && !ramData.empty()) {
    ramData[ramIndex(address)] = value;
  } else if (hasRtc && ramBank >= 0x08 && ramBank <= 0x0C) {
    const std::size_t index = ramBank - 0x08U;
    rtc[index] = value & RTC_MASKS[index];
    latched[index] = rtc[index];
    if (index == 0) {
      rtcCycles = 0;
    }
  }
}

void MBC3::saveBattery(std::ostream& out) const {
  MBC::saveBattery(out);
  if (!hasRtc) {
    return;
  }
  const auto writeLe = [&out](uint64_t value, int bytes) {
    for (int i = 0; i < bytes; i++) {
      out.put(static_cast<char>((value >> (8 * i)) & 0xFF));
    }
  };
  for (const uint8_t reg : rtc) {
    writeLe(reg, 4);
  }
  for (const uint8_t reg : latched) {
    writeLe(reg, 4);
  }
  writeLe(static_cast<uint64_t>(std::time(nullptr)), 8);
}

void MBC3::loadBattery(std::istream& in) {
  MBC::loadBattery(in);
  if (!hasRtc) {
    return;
  }
  constexpr std::streamsize FOOTER_SIZE = 48;
  constexpr std::streamsize FOOTER_SIZE_32BIT_TIME = 44;
  std::array<unsigned char, FOOTER_SIZE> footer{};
  in.read(reinterpret_cast<char*>(footer.data()), FOOTER_SIZE);
  const std::streamsize size = in.gcount();
  if (size < FOOTER_SIZE_32BIT_TIME) {
    return;
  }
  const auto readLe = [&footer](std::size_t offset, int bytes) {
    uint64_t value = 0;
    for (int i = 0; i < bytes; i++) {
      value |=
          static_cast<uint64_t>(footer[offset + static_cast<std::size_t>(i)])
          << (8 * i);
    }
    return value;
  };
  constexpr RtcRegisters RTC_MASKS = {0x3F, 0x3F, 0x1F, 0xFF, 0xC1};
  for (std::size_t i = 0; i < RTC_REGISTERS; i++) {
    rtc[i] = static_cast<uint8_t>(readLe(i * 4, 1) & RTC_MASKS[i]);
    latched[i] =
        static_cast<uint8_t>(readLe((RTC_REGISTERS + i) * 4, 1) & RTC_MASKS[i]);
  }
  const auto savedAt = static_cast<int64_t>(
      readLe(RTC_REGISTERS * 8, size == FOOTER_SIZE ? 8 : 4));
  const auto now = static_cast<int64_t>(std::time(nullptr));
  if (now > savedAt && !rtcHalted()) {
    advanceSeconds(static_cast<uint64_t>(now - savedAt));
  }
}

std::size_t MBC5::ramIndex(uint16_t address) const {
  const auto bank = static_cast<std::size_t>(ramBank % ramBanksCount);
  return bank * 0x2000 + address - 0xA000;
}

uint8_t MBC5::read(uint16_t address) const {
  if (address < 0x4000) {
    return romData[address];
  }
  if (address < 0x8000) {
    return romData[static_cast<std::size_t>(romBank % romBanksCount) * 0x4000 +
                   address - 0x4000];
  }
  if (address >= 0xA000 && address < 0xC000 && ramEnabled && !ramData.empty()) {
    return ramData[ramIndex(address)];
  }
  return 0xFF;
}

void MBC5::write(uint16_t address, uint8_t value) {
  if (address < 0x2000) {
    ramEnabled = (value & 0x0F) == 0x0A;
  } else if (address < 0x3000) {
    romBank = static_cast<uint16_t>((romBank & 0x100) | value);
  } else if (address < 0x4000) {
    romBank = static_cast<uint16_t>((romBank & 0xFF) | ((value & 0x01) << 8));
  } else if (address < 0x6000) {
    ramBank = value & (rumble ? 0x07 : 0x0F);
  } else if (address >= 0xA000 && address < 0xC000 && ramEnabled &&
             !ramData.empty()) {
    ramData[ramIndex(address)] = value;
  }
}

void MBC0::save(std::ostream& out) const { serial::saveFields(out, *this); }

void MBC0::load(std::istream& in) { serial::loadFields(in, *this); }

void MBC1::save(std::ostream& out) const { serial::saveFields(out, *this); }

void MBC1::load(std::istream& in) { serial::loadFields(in, *this); }

void MBC2::save(std::ostream& out) const { serial::saveFields(out, *this); }

void MBC2::load(std::istream& in) {
  serial::loadFields(in, *this);
  romBank = static_cast<uint8_t>(romBank % romBanksCount);
}

void MBC3::save(std::ostream& out) const { serial::saveFields(out, *this); }

void MBC3::load(std::istream& in) {
  serial::loadFields(in, *this);
  romBank = static_cast<uint8_t>(romBank % romBanksCount);
  rtcCycles %= RTC_CYCLES_PER_SECOND;
}

void MBC5::save(std::ostream& out) const { serial::saveFields(out, *this); }

void MBC5::load(std::istream& in) {
  serial::loadFields(in, *this);
  romBank &= 0x1FF;
}
