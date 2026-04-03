#pragma once

#include "./Pipeline.h"
#include "./OamTypes.h"
#include "./Common.h"
#include "./ScanlineContext.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>

class IMemRead;
class InterruptSink;
class LCD;
class PPU
{
  static constexpr int BUFFER_SIZE = XRES * YRES;

public:
  struct State
  {
    std::array<OAM_ENTRY, 40> oamRam;
    std::array<uint8_t, 0x2000> vram;

    uint32_t lineTicks;
    uint8_t windowLine;
    uint8_t lineSpritesCount;
    std::array<OAM_ENTRY, 10> currentLineSprites{};
    ScanlineContext scanlineCtx{};
  };
  PPU(InterruptSink &interruptSink);
  void init();
  void tick();
  [[nodiscard]] uint32_t getCurrentFrame() const;
  void setCurrentFrame(uint32_t frame);
  void setVideoBuffer(const std::array<uint32_t, BUFFER_SIZE> &buffer);

  void oamWrite(uint16_t addr, uint8_t value);
  [[nodiscard]] uint8_t oamRead(uint16_t addr) const;
  [[nodiscard]] uint8_t vramRead(uint16_t address) const;
  void vramWrite(uint16_t address, uint8_t value);

  void setMemRead(IMemRead &memRead) { this->memRead = &memRead; }
  void setLcd(LCD *lcd) { this->lcd = lcd; }
  void setGetTicks(std::function<uint32_t()> fn) { this->getTicksFn = std::move(fn); }
  void setDelay(std::function<void(uint32_t)> fn) { this->delayFn = std::move(fn); }
  [[nodiscard]] const std::array<uint32_t, BUFFER_SIZE> &getVideoBuffer() const;

  [[nodiscard]] PPU::State getState() const;
  void setState(const State &state);

  [[nodiscard]] Pipeline::State getPipelineState() const;
  [[nodiscard]] Pipeline* getPipeline() { return &pipeline; }
  void setPipelineState(const Pipeline::State &state);
  void setFastForward(bool fastForward);
  [[nodiscard]] bool isFastForward() const;

private:
  State state;
  InterruptSink &interruptSink;
  IMemRead *memRead = nullptr;
  LCD *lcd = nullptr;
  Pipeline pipeline;
  std::atomic<bool> fastForward{false};
  std::atomic<uint32_t> currentFrame{0};

  std::array<uint32_t, BUFFER_SIZE> videoBuffers[2]{};
  std::atomic<int> readBufferIndex{0};
  std::array<uint32_t, BUFFER_SIZE> &getWriteBuffer();

  std::function<uint32_t()> getTicksFn;
  std::function<void(uint32_t)> delayFn;

  static constexpr uint16_t VRAM_START_ADDR = 0x8000;
  static constexpr uint16_t OAM_START_ADDR = 0xFE00;
  static constexpr int LINES_PER_FRAME = 154;
  static constexpr int TICKS_PER_LINE = 456;

  uint32_t targetFrameTime = 1000 / 60;
  long prevFrameTime = 0;
  long startTimer = 0;
  long frameCount = 0;

  void incrementLY();
  void loadLineSprites();
  void buildScanlineContext();

  void oamMode();
  void drawingMode();
  void hBlankMode();
  void vBlankMode();
};