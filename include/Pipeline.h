#pragma once

#include "./OamTypes.h"

#include <cstdint>
#include <array>

class PPU;
class Pipeline
{
  constexpr static int BACKGROUND_MAP_DIMENSION = 32;
  constexpr static int PIXEL_TILE_DIMENSION = 8;

  enum class FETCH_STATE
  {
    TILE,
    DATA0,
    DATA1,
    IDLE,
    PUSH
  };
  struct State
  {
    FETCH_STATE fetchState;
    std::array<uint32_t, 16> pixelFifo;
    size_t fifoHead = 0;
    size_t fifoTail = 0;
    size_t fifoSize = 0;
    size_t fifoX = 0;
    uint8_t lineX;
    uint8_t pushedCount;
    uint8_t fetchX;
    std::array<uint8_t, 3> bgwBuffer;
    std::array<uint8_t, 6> objectBuffer;
    std::array<OAM_ENTRY, 3> fetchedEntries;
    uint8_t mapX;
    uint8_t mapY;
    uint8_t tileX;
    uint8_t tileY;
    uint8_t entryCount;
  };

public:
  Pipeline(PPU *ppu);
  void process();
  void reset();
  void oamReset();
  uint8_t getPushedCount();
  bool windowVisible() const;

private:
  PPU *ppu;
  State state;
  void fetch();
  void fetchTile();
  void fetchData0();

  void pushPixel();
  uint8_t calculateMapX() const;
  uint8_t calculateMapY() const;
  uint8_t calculateTileY() const;
  uint32_t bufferIndex() const;

  void fetchData1();
  bool fifoAdd();
  uint32_t fetchSpritePixels(int bit, uint32_t color, uint8_t bgColor);

  uint16_t bgw0ReadAddress() const;
  uint16_t bgw1ReadAddress() const;

  bool fifoIsEmpty() const;
  bool fifoIsFull() const;
  void fifoPush(uint32_t pixel);
  uint32_t fifoPop();

  void loadWindowTile();
  void loadSpriteTile();
  void loadSpriteData(uint8_t offset);
};