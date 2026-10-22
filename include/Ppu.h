// clang-format Language: Cpp
#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <inplace_vector>
#include <optional>
#include <span>
#include <utility>

#include "./Common.h"
#include "./Interrupts.h"
#include "./Lcd.h"
#include "./OamTypes.h"
#include "./Pipeline.h"
#include "./ScanlineContext.h"
#include "./Serializable.h"

class PPU {
  static constexpr int BUFFER_SIZE = XRES * YRES;

 public:
  using Frame = std::array<uint32_t, BUFFER_SIZE>;

  struct State {
    std::array<OAM_ENTRY, 40> oamRam{};
    std::array<uint8_t, 0x2000> vram{};

    LCD::MODE mode = LCD::MODE::VBLANK;
    bool skipOamScan = false;
    uint32_t lineTicks = 0;
    uint8_t windowLine = 0;
    uint32_t mode3Penalty{0};
    std::inplace_vector<OAM_ENTRY, MAX_LINE_SPRITES> lineSprites;
    ScanlineContext scanlineCtx{};
  };
  PPU(Interrupts& interrupts, LCD& lcd);
  void tick();
  [[nodiscard]] uint32_t getCurrentFrame() const;
  [[nodiscard]] bool isOamBlocked() const;
  [[nodiscard]] bool isVramBlocked() const;
  enum class OamBugAccess : uint8_t { Write, Read, ReadIdu };
  bool triggerOamBug(OamBugAccess access);

  void oamWrite(uint16_t addr, uint8_t value);
  [[nodiscard]] uint8_t oamRead(uint16_t addr) const;
  [[nodiscard]] uint8_t vramRead(uint16_t address) const
      pre(address >= VRAM_START && address <= VRAM_END);
  void vramWrite(uint16_t address, uint8_t value)
      pre(address >= VRAM_START && address <= VRAM_END);

  void setOnFrameComplete(std::function<void()> callback) {
    onFrameComplete = std::move(callback);
  }
  [[nodiscard]] const Frame& acquireFrontFrame();
  [[nodiscard]] const Frame& getPublishedFrame() const;

  void save(std::ostream& out) const;
  void load(std::istream& in);

 private:
  State state;
  [[= serial::skip]] Interrupts& interrupts_;
  [[= serial::skip]] LCD& lcd_;
  [[= serial::skip]] std::function<void()> onFrameComplete;
  Pipeline pipeline;
  std::atomic<uint32_t> currentFrame{0};

  [[= serial::skip]] std::array<Frame, 3> frames{};
  [[= serial::skip]] uint8_t backIndex = 0;
  [[= serial::skip]] uint8_t publishedIndex = 1;
  [[= serial::skip]] std::atomic<uint8_t> readyIndex{1};
  [[= serial::skip]] uint8_t frontIndex = 2;

  static constexpr uint16_t VRAM_START_ADDR = 0x8000;
  static constexpr uint16_t OAM_START_ADDR = 0xFE00;
  static constexpr std::size_t OAM_BYTES = 0xA0;
  static constexpr std::size_t OAM_ROW_BYTES = 8;
  static constexpr std::size_t OAM_ROWS = OAM_BYTES / OAM_ROW_BYTES;
  static constexpr int LINES_PER_FRAME = 154;
  static constexpr int TICKS_PER_LINE = 456;
  static constexpr uint32_t LINE_153_LYC_CLEAR_TICKS = 8;
  static constexpr uint32_t LINE_153_LYC_COMPARE_TICKS = 12;
  static constexpr uint32_t BOOT_LINE_TICKS = 400;
  static constexpr uint32_t STAT_MODE_DELAY = 4;
  static constexpr uint32_t LCD_ON_LINE_TICKS = 4;
  static constexpr uint32_t LCD_ON_MODE3_DELAY = 2;
  static constexpr uint32_t SPRITE_FETCH_PENALTY = 6;
  static constexpr uint32_t WINDOW_PENALTY = 6;
  static constexpr uint8_t MAX_WINDOW_X = 166;
  static constexpr uint8_t FRAME_INDEX_MASK = 0x3;
  static constexpr uint8_t FRAME_FRESH_BIT = 0x4;

  void publishFrame()
      post(backIndex < frames.size() && backIndex != publishedIndex);
  void finishFrame();
  void setMode(LCD::MODE mode);
  void startLine();
  void incrementLY();
  void compareLyc(uint8_t ly);
  [[nodiscard]] std::optional<std::size_t> oamScanRow() const;
  [[nodiscard]] std::span<uint8_t, OAM_BYTES> oamBytes();
  void loadLineSprites();
  [[nodiscard]] uint32_t calculateSpritePenalty() const;
  [[nodiscard]] uint32_t calculateWindowPenalty() const;
  void requestStatInterrupt(LCD::LCDS_SRC source);
  void enterVBlank();
  void buildScanlineContext();

  void oamMode();
  void drawingMode();
  void hBlankMode();
  void vBlankMode();
};
