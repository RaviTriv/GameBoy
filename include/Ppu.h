#pragma once

#include "./Pipeline.h"
#include "./OamTypes.h"
#include "./Common.h"

#include <array>
#include <cstdint>
#include <memory>
#include <list>

class Bus;
class CPU;
class LCD;
class UI;
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
    std::list<OAM_ENTRY> currentLineSprites;
    std::array<uint32_t, BUFFER_SIZE> videoBuffer;

    uint32_t currentFrame;
  };
  PPU(std::shared_ptr<Bus> bus, std::shared_ptr<CPU> cpu, std::shared_ptr<LCD> lcd, std::shared_ptr<UI> ui);
  void init();
  void tick();
  uint32_t getCurrentFrame();

  void oamWrite(uint16_t addr, uint8_t value);
  uint8_t oamRead(uint16_t addr);
  uint8_t vramRead(uint16_t address) const;
  void vramWrite(uint16_t address, uint8_t value);

  void setCpu(std::shared_ptr<CPU> cpu);
  void setBus(std::shared_ptr<Bus> bus);
  void setUi(std::shared_ptr<UI> ui);
  const std::array<uint32_t, BUFFER_SIZE> &getVideoBuffer() const;

  PPU::State getState() const;
  void setState(const State &state);

  Pipeline::State getPipelineState() const;
  Pipeline* getPipeline() { return &pipeline; }
  void setPipelineState(const Pipeline::State &state);
  void setFastForward(bool fastForward);
  bool isFastForward() const;

private:
  State state;
  std::shared_ptr<Bus> bus;
  std::shared_ptr<CPU> cpu;
  std::shared_ptr<LCD> lcd;
  std::shared_ptr<UI> ui;
  friend class Pipeline;
  Pipeline pipeline;
  bool fastForward;

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

  void oamMode();
  void drawingMode();
  void hBlankMode();
  void vBlankMode();
};