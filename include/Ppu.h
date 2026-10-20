#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <utility>

#include "./Common.h"
#include "./InterruptRegs.h"
#include "./OamTypes.h"
#include "./Pipeline.h"
#include "./ScanlineContext.h"
#include "./Serializable.h"

class LCD;
class PPU : public Serializable {
  static constexpr int BUFFER_SIZE = XRES * YRES;

 public:
  using Frame = std::array<uint32_t, BUFFER_SIZE>;

  struct State {
    std::array<OAM_ENTRY, 40> oamRam{};
    std::array<uint8_t, 0x2000> vram{};

    uint32_t lineTicks = 0;
    uint8_t windowLine = 0;
    uint8_t lineSpritesCount = 0;
    uint32_t spritePenalty{0};
    std::array<OAM_ENTRY, 10> currentLineSprites{};
    ScanlineContext scanlineCtx{};
  };
  explicit PPU(InterruptRegs interruptRegs);
  void init();
  void tick();
  [[nodiscard]] uint32_t getCurrentFrame() const;

  void oamWrite(uint16_t addr, uint8_t value);
  [[nodiscard]] uint8_t oamRead(uint16_t addr) const;
  [[nodiscard]] uint8_t vramRead(uint16_t address) const;
  void vramWrite(uint16_t address, uint8_t value);

  void setLcd(LCD* newLcd) { lcd = newLcd; }
  void setOnFrameComplete(std::function<void()> callback) {
    onFrameComplete = std::move(callback);
  }
  [[nodiscard]] const Frame& acquireFrontFrame();
  [[nodiscard]] const Frame& getPublishedFrame() const;

  void save(std::ostream& out) const override;
  void load(std::istream& in) override;

 private:
  State state;
  InterruptRegs interruptRegs_;
  LCD* lcd = nullptr;
  std::function<void()> onFrameComplete;
  Pipeline pipeline;
  std::atomic<uint32_t> currentFrame{0};

  std::array<Frame, 3> frames{};
  uint8_t backIndex = 0;
  uint8_t publishedIndex = 1;
  std::atomic<uint8_t> readyIndex{1};
  uint8_t frontIndex = 2;

  static constexpr uint16_t VRAM_START_ADDR = 0x8000;
  static constexpr uint16_t OAM_START_ADDR = 0xFE00;
  static constexpr int LINES_PER_FRAME = 154;
  static constexpr int TICKS_PER_LINE = 456;
  static constexpr uint8_t FRAME_INDEX_MASK = 0x3;
  static constexpr uint8_t FRAME_FRESH_BIT = 0x4;

  void publishFrame();
  void finishFrame();
  void incrementLY();
  void loadLineSprites();
  void buildScanlineContext();

  void oamMode();
  void drawingMode();
  void hBlankMode();
  void vBlankMode();
};
