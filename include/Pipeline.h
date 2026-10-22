// clang-format Language: Cpp
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <inplace_vector>
#include <istream>
#include <ostream>
#include <span>

#include "./Common.h"
#include "./OamTypes.h"
#include "./PixelFifo.h"
#include "./ScanlineContext.h"
#include "./Serializable.h"

class Pipeline {
  constexpr static int BACKGROUND_MAP_DIMENSION = 32;
  constexpr static int PIXEL_TILE_DIMENSION = 8;
  constexpr static std::size_t MAX_LINE_SPRITES = 10;

  enum class FETCH_STATE { TILE, DATA0, DATA1, PUSH };

 public:
  using VramView = std::span<const uint8_t, 0x2000>;
  using FrameView = std::span<uint32_t, static_cast<std::size_t>(XRES) * YRES>;

  struct State {
    FETCH_STATE fetchState = FETCH_STATE::TILE;
    size_t fifoX = 0;
    uint8_t lineX = 0;
    uint8_t pushedCount = 0;
    uint8_t fetchX = 0;
    std::array<uint8_t, 3> bgwBuffer{};
    std::array<uint8_t, 20> objectBuffer{};
    std::inplace_vector<OAM_ENTRY, MAX_LINE_SPRITES> fetchedEntries;
    uint8_t mapX = 0;
    uint8_t mapY = 0;
    uint8_t tileX = 0;
    uint8_t tileY = 0;
  };
  explicit Pipeline(VramView vram);
  void beginScanline(const ScanlineContext& ctx, FrameView target)
      pre(ctx.ly < YRES && ctx.lineTicks != nullptr &&
          ctx.sprites.size() <= MAX_LINE_SPRITES);
  void process();
  void reset();
  void oamReset();
  [[nodiscard]] bool isWindowVisible() const;
  void save(std::ostream& out) const;
  void load(std::istream& in);
  [[nodiscard]] uint8_t getPushedCount() const { return state.pushedCount; }

 private:
  State state;
  [[= serial::skip]] VramView vram_;
  [[= serial::skip]] std::span<uint32_t> target_;
  [[= serial::skip]] const ScanlineContext* ctx = nullptr;
  PixelFifo pixelFifo;
  void fetch();
  void fetchTile();
  void fetchDataLow();
  void pushPixel();
  uint8_t calculateMapX() const;
  uint8_t calculateMapY() const;
  uint8_t calculateTileY() const;
  uint32_t bufferIndex() const;
  [[nodiscard]] uint8_t readVram(uint16_t address) const
      pre(address >= VRAM_START && address <= VRAM_END);
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
