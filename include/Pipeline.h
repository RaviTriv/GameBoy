#pragma once

#include "./OamTypes.h"
#include "./PixelFifo.h"
#include "./ScanlineContext.h"

#include <cstdint>
#include <array>
#include <functional>

using ReadFn = std::function<uint8_t(uint16_t)>;
using WritePixelFn = std::function<void(uint32_t, uint32_t)>;

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

public:
  struct State
  {
    FETCH_STATE fetchState = FETCH_STATE::TILE;
    size_t fifoX = 0;
    uint8_t lineX = 0;
    uint8_t pushedCount = 0;
    uint8_t fetchX = 0;
    std::array<uint8_t, 3> bgwBuffer{};
    std::array<uint8_t, 20> objectBuffer{};
    std::array<OAM_ENTRY, 10> fetchedEntries{};
    uint8_t mapX = 0;
    uint8_t mapY = 0;
    uint8_t tileX = 0;
    uint8_t tileY = 0;
    uint8_t entryCount = 0;
  };
  Pipeline(ReadFn readFn, WritePixelFn writePixelFn);
  void beginScanline(const ScanlineContext &ctx);
  void process();
  void reset();
  void oamReset();
  [[nodiscard]] bool isWindowVisible() const;
  [[nodiscard]] const State &getState() const { return state; }
  void setState(const State &s) { state = s; }
  [[nodiscard]] const PixelFifo *getPixelFifo() const { return &pixelFifo; }
  [[nodiscard]] PixelFifo *getPixelFifo() { return &pixelFifo; }
  [[nodiscard]] uint8_t getPushedCount() const { return state.pushedCount; }

private:
  State state;
  ReadFn readFn;
  WritePixelFn writePixelFn;
  const ScanlineContext *ctx = nullptr;
  PixelFifo pixelFifo;
  void fetch();
  void fetchTile();
  void fetchDataLow();
  void pushPixel();
  uint8_t calculateMapX() const;
  uint8_t calculateMapY() const;
  uint8_t calculateTileY() const;
  uint32_t bufferIndex() const;
  uint16_t bgw0ReadAddress() const;
  uint16_t bgw1ReadAddress() const;
  uint16_t windowTileReadAddress(uint8_t wTileY) const;
  void loadWindowTile();
  void loadSpriteTile();
  void loadSpriteData(uint8_t offset);
  void fetchDataHigh();
  bool processTile();
  uint32_t fetchSpritePixels(int bit, uint32_t color, uint8_t bgColor);
};
