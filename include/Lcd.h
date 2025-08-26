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

  struct LCDCBits
  {
    uint8_t bgWindowEnablePriority : 1;
    uint8_t objEnable : 1;
    uint8_t objSize : 1;
    uint8_t bgTileMap : 1;
    uint8_t bgWindowTiles : 1;
    uint8_t windowEnable : 1;
    uint8_t windowTileMap : 1;
    uint8_t lcdEnable : 1;
  };

  struct LCDSBits
  {
    uint8_t ppuMode : 2;
    uint8_t lycFlag : 1;
    uint8_t mode0Select : 1;
    uint8_t mode1Select : 1;
    uint8_t mode2Select : 1;
    uint8_t lycSelect : 1;
  };

  struct State
  {

    union
    {
      LCDCBits lcdcBits;
      uint8_t lcdc;
    };
    union
    {
      LCDSBits lcdsBits;
      uint8_t lcds;
    };
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
  LCD(std::shared_ptr<DMA> dma);
  uint8_t read(uint16_t address);
  void write(uint16_t address, uint8_t value);
  bool isLcdStatIntEnabled(uint8_t source);
  uint16_t getBgMapArea() const;
  uint16_t getBgWindowDataArea() const;
  uint16_t getWindowMapArea() const;

private:
  State state;
  std::shared_ptr<DMA> dma;
  static constexpr std::array<unsigned long, 4> defaultColors = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000};
  void updatePalettes(PaletteType type, uint8_t value);
};