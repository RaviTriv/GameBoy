#pragma once

#include <array>
#include <cstdint>
#include <memory>

class DMA;
class LCD
{
  friend class PPU;
  friend class Pipeline;
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

public:
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
    uint8_t lcdc;
    uint8_t lcds;
    uint8_t scrollX;
    uint8_t scrollY;
    uint8_t ly;
    uint8_t lyCompare;
    uint8_t dma;
    union
    {
      struct
      {
        uint8_t colorId0 : 2;
        uint8_t colorId1 : 2;
        uint8_t colorId2 : 2;
        uint8_t colorId3 : 2;
      };
      uint8_t bgp;
    };
    std::array<PaletteRegister, 3> palettes;
    uint8_t windowX;
    uint8_t windowY;
    std::array<uint32_t, 4> bgColors;
    std::array<uint32_t, 4> ob1Colors;
    std::array<uint32_t, 4> ob2Colors;
  };

  LCD(std::shared_ptr<DMA> dma);
  uint8_t read(uint16_t address);
  void write(uint16_t address, uint8_t value);
  bool isLcdStatIntEnabled(uint8_t source);
  uint16_t getBgMapArea() const;
  uint16_t getBgWindowDataArea() const;
  uint16_t getWindowMapArea() const;
  uint8_t getObjHeight();
  bool isWindowEnabled();
  bool isBgWindowEnabled();
  bool isObjEnabled();
  int getLcdMode() const;
  void setLcdMode(MODE mode);
  bool isLycFlag();
  void setLycFlag(bool value);
  LCD::State getState() const;
  void setState(const State &state);

private:
  State state;
  std::shared_ptr<DMA> dma;
  static constexpr std::array<unsigned long, 4> defaultColors = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000};
  void updatePalettes(PaletteType type, uint8_t value);
  bool getBit(uint8_t value, int bit) const;
  void setBit(uint8_t &value, int bit, bool set);
};