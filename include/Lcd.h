#pragma once

#include <array>
#include <cstdint>
#include <functional>

class LCD
{
public:
  enum PaletteType
  {
    BGP = 0,
    OBP0 = 1,
    OBP1 = 2
  };

  enum MODE
  {
    HBLANK,
    VBLANK,
    OAM,
    DRAWING
  };

  enum LCDS_SRC
  {
    S_HBLANK = (1 << 3),
    S_VBLANK = (1 << 4),
    S_OAM = (1 << 5),
    S_LYC = (1 << 6),
  };

  union PaletteRegister
  {
    struct
    {
      uint8_t colorId0 : 2;
      uint8_t colorId1 : 2;
      uint8_t colorId2 : 2;
      uint8_t colorId3 : 2;
    };
    uint8_t palette;
  };
  struct State
  {
    uint8_t lcdc = 0;
    uint8_t lcds = 0;
    uint8_t scrollX = 0;
    uint8_t scrollY = 0;
    uint8_t ly = 0;
    uint8_t lyCompare = 0;
    uint8_t dma = 0;
    union
    {
      struct
      {
        uint8_t colorId0 : 2;
        uint8_t colorId1 : 2;
        uint8_t colorId2 : 2;
        uint8_t colorId3 : 2;
      };
      uint8_t bgp = 0;
    };
    std::array<PaletteRegister, 3> palettes{};
    uint8_t windowX = 0;
    uint8_t windowY = 0;
    std::array<uint32_t, 4> bgColors{};
    std::array<uint32_t, 4> ob1Colors{};
    std::array<uint32_t, 4> ob2Colors{};
  };

  LCD(std::function<void(uint8_t)> onDmaStart);
  [[nodiscard]] uint8_t read(uint16_t address) const;
  void write(uint16_t address, uint8_t value);
  [[nodiscard]] bool isLcdStatIntEnabled(uint8_t source) const;
  [[nodiscard]] uint16_t getBgMapArea() const;
  [[nodiscard]] uint16_t getBgWindowDataArea() const;
  [[nodiscard]] uint16_t getWindowMapArea() const;
  [[nodiscard]] uint8_t getObjHeight() const;
  [[nodiscard]] bool isWindowEnabled() const;
  [[nodiscard]] bool isBgWindowEnabled() const;
  [[nodiscard]] bool isObjEnabled() const;
  [[nodiscard]] int getLcdMode() const;
  void setLcdMode(MODE mode);
  [[nodiscard]] bool isLycFlag() const;
  void setLycFlag(bool value);
  [[nodiscard]] LCD::State getState() const;
  void setState(const State &state);

  [[nodiscard]] uint8_t getLy() const;
  void setLy(uint8_t value);
  void incrementLy();
  [[nodiscard]] uint8_t getLyCompare() const;
  [[nodiscard]] uint8_t getScrollX() const;
  [[nodiscard]] uint8_t getScrollY() const;
  [[nodiscard]] uint8_t getWindowX() const;
  [[nodiscard]] uint8_t getWindowY() const;
  [[nodiscard]] const std::array<uint32_t, 4> &getBgColors() const;
  [[nodiscard]] const std::array<uint32_t, 4> &getOb1Colors() const;
  [[nodiscard]] const std::array<uint32_t, 4> &getOb2Colors() const;
  [[nodiscard]] uint8_t getLcds() const;

private:
  State state;
  std::function<void(uint8_t)> onDmaStart;
  static constexpr std::array<unsigned long, 4> defaultColors = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000};
  void updatePalettes(PaletteType type, uint8_t value);
  bool getBit(uint8_t value, int bit) const;
  void setBit(uint8_t &value, int bit, bool set);
};
