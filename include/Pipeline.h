// clang-format Language: Cpp
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <span>

#include "./Common.h"
#include "./OamTypes.h"
#include "./ScanlineContext.h"
#include "./Serializable.h"

class Pipeline {
  constexpr static int BACKGROUND_MAP_DIMENSION = 32;
  constexpr static int PIXEL_TILE_DIMENSION = 8;
  constexpr static std::size_t MAX_LINE_SPRITES = 10;
  constexpr static uint8_t MAX_WINDOW_X = 166;
  constexpr static std::size_t WINDOW_X_DELAY = 2;

  enum class FETCH_STATE : uint8_t {
    TILE0,
    TILE1,
    DATA0_0,
    DATA0_1,
    DATA1_0,
    DATA1_1,
    PUSH
  };
  enum class OBJ_STATE : uint8_t {
    IDLE,
    WAIT,
    ATTR0,
    ATTR1,
    DATA0_0,
    DATA0_1,
    DATA1
  };

 public:
  using VramView = std::span<const uint8_t, 0x2000>;
  using FrameView = std::span<uint32_t, static_cast<std::size_t>(XRES) * YRES>;

  struct State {
    FETCH_STATE fetchState = FETCH_STATE::TILE0;
    OBJ_STATE objState = OBJ_STATE::IDLE;
    int16_t lineX = -16;
    uint8_t pushedCount = 0;
    uint8_t startDelay = 0;
    uint16_t fetchAddress = 0;
    uint8_t tileIndex = 0;
    uint8_t tileLow = 0;
    uint8_t tileHigh = 0;
    uint8_t nextSprite = 0;
    uint8_t spriteLow = 0;
    uint8_t spriteHigh = 0;
    uint8_t bgLow = 0;
    uint8_t bgHigh = 0;
    uint8_t bgCount = 0;
    std::array<uint8_t, PIXEL_TILE_DIMENSION> objFifo{};
    uint8_t objHead = 0;
    uint8_t objCount = 0;
    uint8_t windowLine = 0xFF;
    uint8_t windowTileX = 0;
    std::array<uint8_t, WINDOW_X_DELAY> windowXHistory{};
    uint8_t windowX = 0;
    uint8_t bgPalette = 0;
    uint8_t obj0Palette = 0;
    uint8_t obj1Palette = 0;
    uint8_t lateBgPalette = 0;
    uint8_t lateObj0Palette = 0;
    uint8_t lateObj1Palette = 0;
    bool lateWindowEnabled = false;
    bool lateBgEnabled = false;
    bool lateObjEnabled = false;
    bool windowEnabled = false;
    bool bgEnabled = false;
    bool objEnabled = false;
    bool wyTriggered = false;
    bool wxTriggered = false;
    bool windowFetching = false;
    bool insertBgPixel = false;
    bool insertionGlitchDisabled = false;
    bool stalled = false;
    bool lineDone = false;
  };
  explicit Pipeline(VramView vram);
  void beginScanline(const ScanlineContext& ctx, FrameView target)
      pre(ctx.ly < YRES && ctx.lineTicks != nullptr &&
          ctx.sprites.size() <= MAX_LINE_SPRITES);
  void process();
  void reset();
  void oamReset();
  void newFrame();
  void checkWindowY(uint8_t ly, uint8_t windowY, bool windowEnabled);
  void save(std::ostream& out) const;
  void load(std::istream& in);
  [[nodiscard]] bool isLineDone() const { return state.lineDone; }
  [[nodiscard]] bool isWindowYTriggered() const { return state.wyTriggered; }
  [[nodiscard]] bool finishesWithin(int dots) const {
    return state.lineDone ||
           (state.lineX + dots >= XRES && previewFinishes(dots));
  }

 private:
  State state;
  [[= serial::skip]] VramView vram_;
  [[= serial::skip]] std::span<uint32_t> target_;
  [[= serial::skip]] const ScanlineContext* ctx = nullptr;
  [[= serial::skip]] bool preview_ = false;
  [[nodiscard]] uint8_t readVram(uint16_t address) const
      pre(address >= VRAM_START && address <= VRAM_END);
  [[nodiscard]] bool previewFinishes(int dots) const;
  bool latchRegisters();
  bool checkWindowTrigger(bool windowXChanged);
  bool matchSprite();
  void stepSpriteFetch();
  void advanceFetcher();
  void renderPixel();
  void pushBgRow();
  void overlaySpriteRow();
  [[nodiscard]] uint8_t fetcherY() const;
  [[nodiscard]] uint16_t tileMapAddress() const;
  [[nodiscard]] uint16_t tileDataAddress() const;
  [[nodiscard]] uint16_t spriteDataAddress() const;
  [[nodiscard]] uint8_t spriteMatchX() const;
};
