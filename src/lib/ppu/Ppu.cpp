#include "Ppu.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>

#include "Common.h"
#include "IMemRead.h"
#include "InterruptRegs.h"
#include "Lcd.h"
#include "Logger.h"
#include "OamTypes.h"
#include "Serializable.h"

PPU::PPU(InterruptRegs interruptRegs)
    : interruptRegs_(interruptRegs),
      pipeline(
          [this](uint16_t addr) -> uint8_t { return memRead->read8(addr); },
          [this](uint32_t idx, uint32_t color) {
            getWriteBuffer()[idx] = color;
          }) {}

void PPU::init() { lcd->setLcdMode(LCD::MODE::OAM); }

const std::array<uint32_t, PPU::BUFFER_SIZE>& PPU::getVideoBuffer() const {
  return videoBuffers[readBufferIndex.load(std::memory_order_acquire)];
}

std::array<uint32_t, PPU::BUFFER_SIZE>& PPU::getWriteBuffer() {
  return videoBuffers[1 - readBufferIndex.load(std::memory_order_relaxed)];
}

void PPU::tick() {
  switch (lcd->consumeEnableTransition()) {
    case LCD::TURNED_OFF:
      lcd->setLy(0);
      lcd->setLcdMode(LCD::MODE::HBLANK);
      pipeline.reset();
      state.lineTicks = 0;
      state.windowLine = 0;
      getWriteBuffer().fill(0xFFFFFFFF);
      currentFrame.fetch_add(1, std::memory_order_relaxed);
      readBufferIndex.store(1 - readBufferIndex.load(std::memory_order_relaxed),
                            std::memory_order_release);
      return;
    case LCD::TURNED_ON:
      lcd->setLy(0);
      lcd->setLcdMode(LCD::MODE::OAM);
      state.lineTicks = 0;
      state.windowLine = 0;
      break;
    case LCD::NO_TRANSITION:
      break;
  }

  if (!lcd->isLcdEnabled()) {
    return;
  }

  state.lineTicks++;
  switch (lcd->getLcdMode()) {
    case LCD::MODE::OAM:
      oamMode();
      break;
    case LCD::MODE::DRAWING:
      drawingMode();
      break;
    case LCD::MODE::HBLANK:
      hBlankMode();
      break;
    case LCD::MODE::VBLANK:
      vBlankMode();
      break;
    default:
      Logger::GetLogger()->error("Unknown PPU mode: {}", lcd->getLcdMode());
      break;
  }
}

void PPU::oamWrite(uint16_t address, uint8_t value) {
  if (address >= OAM_START_ADDR) {
    address -= OAM_START_ADDR;
  }
  if (address >= 0xA0) {
    return;
  }
  auto* p = reinterpret_cast<uint8_t*>(state.oamRam.data());
  p[address] = value;
}

uint8_t PPU::oamRead(uint16_t address) const {
  if (address >= OAM_START_ADDR) {
    address -= OAM_START_ADDR;
  }
  if (address >= 0xA0) {
    return 0xFF;
  }
  const auto* p = reinterpret_cast<const uint8_t*>(state.oamRam.data());
  return p[address];
}

void PPU::vramWrite(uint16_t address, uint8_t value) {
  state.vram[address - VRAM_START_ADDR] = value;
}

uint8_t PPU::vramRead(uint16_t address) const {
  return state.vram[address - VRAM_START_ADDR];
}

/*
<-----PPU-SM-START----->
*/

void PPU::incrementLY() {
  if (pipeline.isWindowVisible() && lcd->getLy() >= lcd->getWindowY() &&
      lcd->getLy() < lcd->getWindowY() + YRES) {
    state.windowLine++;
  }

  lcd->incrementLy();

  if (lcd->getLy() == lcd->getLyCompare()) {
    lcd->setLycFlag(true);

    if (lcd->isLcdStatIntEnabled(static_cast<uint8_t>(LCD::LCDS_SRC::S_LYC))) {
      interruptRegs_.request(InterruptType::LCD_STAT);
    }
  } else {
    lcd->setLycFlag(false);
  }
}

void PPU::loadLineSprites() {
  int curY = lcd->getLy();

  uint8_t spriteHeight = lcd->getObjHeight();

  for (std::size_t i = 0; i < 40; i++) {
    OAM_ENTRY entry = state.oamRam[i];

    if (state.lineSpritesCount >= 10) {
      break;
    }

    if (entry.y <= curY + 16 && entry.y + spriteHeight > curY + 16) {
      uint8_t insertPos = 0;
      while (insertPos < state.lineSpritesCount &&
             state.currentLineSprites[insertPos].x <= entry.x) {
        insertPos++;
      }

      for (uint8_t j = state.lineSpritesCount; j > insertPos; j--) {
        state.currentLineSprites[j] = state.currentLineSprites[j - 1];
      }

      state.currentLineSprites[insertPos] = entry;
      state.lineSpritesCount++;
    }
  }
}

void PPU::buildScanlineContext() {
  state.scanlineCtx.scrollX = lcd->getScrollX();
  state.scanlineCtx.scrollY = lcd->getScrollY();
  state.scanlineCtx.ly = lcd->getLy();
  state.scanlineCtx.windowX = lcd->getWindowX();
  state.scanlineCtx.windowY = lcd->getWindowY();
  state.scanlineCtx.windowLine = state.windowLine;
  state.scanlineCtx.bgWindowEnabled = lcd->isBgWindowEnabled();
  state.scanlineCtx.objEnabled = lcd->isObjEnabled();
  state.scanlineCtx.windowEnabled = lcd->isWindowEnabled();
  state.scanlineCtx.objHeight = lcd->getObjHeight();
  state.scanlineCtx.bgMapArea = lcd->getBgMapArea();
  state.scanlineCtx.bgWinDataArea = lcd->getBgWindowDataArea();
  state.scanlineCtx.winMapArea = lcd->getWindowMapArea();
  state.scanlineCtx.bgColors = lcd->getBgColors();
  state.scanlineCtx.ob1Colors = lcd->getOb1Colors();
  state.scanlineCtx.ob2Colors = lcd->getOb2Colors();
  state.scanlineCtx.sprites = state.currentLineSprites.data();
  state.scanlineCtx.spriteCount = state.lineSpritesCount;
  state.scanlineCtx.lineTicks = &state.lineTicks;
}

void PPU::oamMode() {
  if (state.lineTicks == 1) {
    state.lineSpritesCount = 0;
    loadLineSprites();
  }

  if (state.lineTicks >= 80) {
    lcd->setLcdMode(LCD::MODE::DRAWING);
    state.spritePenalty = state.lineSpritesCount * 6;

    buildScanlineContext();
    pipeline.beginScanline(state.scanlineCtx);
    pipeline.oamReset();
  }
}

void PPU::drawingMode() {
  if (pipeline.getPushedCount() < XRES) {
    pipeline.process();
  }

  if (pipeline.getPushedCount() >= XRES) {
    if (state.spritePenalty > 0) {
      state.spritePenalty--;
      return;
    }

    pipeline.reset();
    lcd->setLcdMode(LCD::MODE::HBLANK);

    if (lcd->isLcdStatIntEnabled(
            static_cast<uint8_t>(LCD::LCDS_SRC::S_HBLANK))) {
      interruptRegs_.request(InterruptType::LCD_STAT);
    }
  }
}

void PPU::hBlankMode() {
  if (state.lineTicks >= TICKS_PER_LINE) {
    incrementLY();

    if (lcd->getLy() >= YRES) {
      lcd->setLcdMode(LCD::MODE::VBLANK);

      interruptRegs_.request(InterruptType::VBLANK);

      if (lcd->isLcdStatIntEnabled(
              static_cast<uint8_t>(LCD::LCDS_SRC::S_VBLANK))) {
        interruptRegs_.request(InterruptType::LCD_STAT);
      }

      currentFrame.fetch_add(1, std::memory_order_relaxed);
      readBufferIndex.store(1 - readBufferIndex.load(std::memory_order_relaxed),
                            std::memory_order_release);
    } else {
      lcd->setLcdMode(LCD::MODE::OAM);
    }

    state.lineTicks = 0;
  }
}

void PPU::vBlankMode() {
  if (state.lineTicks >= TICKS_PER_LINE) {
    incrementLY();

    if (lcd->getLy() >= LINES_PER_FRAME) {
      lcd->setLcdMode(LCD::MODE::OAM);
      lcd->setLy(0);
      state.windowLine = 0;
    }
    state.lineTicks = 0;
  }
}

/*
<-----PPU-SM-END----->
*/

uint32_t PPU::getCurrentFrame() const {
  return currentFrame.load(std::memory_order_relaxed);
}

void PPU::save(std::ostream& out) const {
  State saved = state;
  saved.scanlineCtx.sprites = nullptr;
  saved.scanlineCtx.lineTicks = nullptr;
  serial::write(out, saved);
  serial::write(out, getCurrentFrame());
  serial::write(out, getVideoBuffer());
  pipeline.save(out);
}

void PPU::load(std::istream& in) {
  serial::read(in, state);
  if (state.lineSpritesCount > state.currentLineSprites.size() ||
      state.scanlineCtx.spriteCount > state.currentLineSprites.size()) {
    throw std::runtime_error("Invalid PPU sprite count in save file");
  }
  state.scanlineCtx.sprites = state.currentLineSprites.data();
  state.scanlineCtx.lineTicks = &state.lineTicks;

  uint32_t frame = 0;
  serial::read(in, frame);
  currentFrame.store(frame, std::memory_order_relaxed);

  serial::read(in, videoBuffers[0]);
  videoBuffers[1] = videoBuffers[0];

  pipeline.load(in);
  pipeline.beginScanline(state.scanlineCtx);
}
