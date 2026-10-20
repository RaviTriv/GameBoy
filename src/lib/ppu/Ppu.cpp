#include "Ppu.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>

#include "Common.h"
#include "Interrupts.h"
#include "Lcd.h"
#include "Logger.h"
#include "OamTypes.h"
#include "Serializable.h"

PPU::PPU(Interrupts& interrupts, LCD& lcd)
    : interrupts_(interrupts), lcd_(lcd), pipeline(state.vram) {
  lcd_.setLcdMode(LCD::MODE::OAM);
}

const PPU::Frame& PPU::acquireFrontFrame() {
  if ((readyIndex.load(std::memory_order_relaxed) & FRAME_FRESH_BIT) != 0) {
    frontIndex = readyIndex.exchange(frontIndex, std::memory_order_acq_rel) &
                 FRAME_INDEX_MASK;
  }
  return frames[frontIndex];
}

const PPU::Frame& PPU::getPublishedFrame() const {
  return frames[publishedIndex];
}

void PPU::finishFrame() {
  publishFrame();
  currentFrame.fetch_add(1, std::memory_order_relaxed);
  if (onFrameComplete) {
    onFrameComplete();
  }
}

void PPU::publishFrame() {
  publishedIndex = backIndex;
  backIndex = readyIndex.exchange(backIndex | FRAME_FRESH_BIT,
                                  std::memory_order_acq_rel) &
              FRAME_INDEX_MASK;
}

void PPU::tick() {
  switch (lcd_.consumeEnableTransition()) {
    case LCD::TURNED_OFF:
      lcd_.setLy(0);
      lcd_.setLcdMode(LCD::MODE::HBLANK);
      pipeline.reset();
      state.lineTicks = 0;
      state.windowLine = 0;
      frames[backIndex].fill(0xFFFFFFFF);
      finishFrame();
      return;
    case LCD::TURNED_ON:
      lcd_.setLy(0);
      lcd_.setLcdMode(LCD::MODE::OAM);
      state.lineTicks = 0;
      state.windowLine = 0;
      break;
    case LCD::NO_TRANSITION:
      break;
  }

  if (!lcd_.isLcdEnabled()) {
    return;
  }

  state.lineTicks++;
  switch (lcd_.getLcdMode()) {
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
      Logger::GetLogger()->error("Unknown PPU mode: {}", lcd_.getLcdMode());
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
  if (pipeline.isWindowVisible() && lcd_.getLy() >= lcd_.getWindowY() &&
      lcd_.getLy() < lcd_.getWindowY() + YRES) {
    state.windowLine++;
  }

  lcd_.incrementLy();

  if (lcd_.getLy() == lcd_.getLyCompare()) {
    lcd_.setLycFlag(true);

    if (lcd_.isLcdStatIntEnabled(static_cast<uint8_t>(LCD::LCDS_SRC::S_LYC))) {
      interrupts_.request(InterruptType::LCD_STAT);
    }
  } else {
    lcd_.setLycFlag(false);
  }
}

void PPU::loadLineSprites() {
  int curY = lcd_.getLy();

  uint8_t spriteHeight = lcd_.getObjHeight();

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
  state.scanlineCtx.scrollX = lcd_.getScrollX();
  state.scanlineCtx.scrollY = lcd_.getScrollY();
  state.scanlineCtx.ly = lcd_.getLy();
  state.scanlineCtx.windowX = lcd_.getWindowX();
  state.scanlineCtx.windowY = lcd_.getWindowY();
  state.scanlineCtx.windowLine = state.windowLine;
  state.scanlineCtx.bgWindowEnabled = lcd_.isBgWindowEnabled();
  state.scanlineCtx.objEnabled = lcd_.isObjEnabled();
  state.scanlineCtx.windowEnabled = lcd_.isWindowEnabled();
  state.scanlineCtx.objHeight = lcd_.getObjHeight();
  state.scanlineCtx.bgMapArea = lcd_.getBgMapArea();
  state.scanlineCtx.bgWinDataArea = lcd_.getBgWindowDataArea();
  state.scanlineCtx.winMapArea = lcd_.getWindowMapArea();
  state.scanlineCtx.bgColors = lcd_.getBgColors();
  state.scanlineCtx.ob1Colors = lcd_.getOb1Colors();
  state.scanlineCtx.ob2Colors = lcd_.getOb2Colors();
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
    lcd_.setLcdMode(LCD::MODE::DRAWING);
    state.spritePenalty = state.lineSpritesCount * 6;

    buildScanlineContext();
    pipeline.beginScanline(state.scanlineCtx, frames[backIndex]);
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
    lcd_.setLcdMode(LCD::MODE::HBLANK);

    if (lcd_.isLcdStatIntEnabled(
            static_cast<uint8_t>(LCD::LCDS_SRC::S_HBLANK))) {
      interrupts_.request(InterruptType::LCD_STAT);
    }
  }
}

void PPU::hBlankMode() {
  if (state.lineTicks >= TICKS_PER_LINE) {
    incrementLY();

    if (lcd_.getLy() >= YRES) {
      lcd_.setLcdMode(LCD::MODE::VBLANK);

      interrupts_.request(InterruptType::VBLANK);

      if (lcd_.isLcdStatIntEnabled(
              static_cast<uint8_t>(LCD::LCDS_SRC::S_VBLANK))) {
        interrupts_.request(InterruptType::LCD_STAT);
      }

      finishFrame();
    } else {
      lcd_.setLcdMode(LCD::MODE::OAM);
    }

    state.lineTicks = 0;
  }
}

void PPU::vBlankMode() {
  if (state.lineTicks >= TICKS_PER_LINE) {
    incrementLY();

    if (lcd_.getLy() >= LINES_PER_FRAME) {
      lcd_.setLcdMode(LCD::MODE::OAM);
      lcd_.setLy(0);
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
  serial::write(out, getPublishedFrame());
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

  serial::read(in, frames[backIndex]);
  publishFrame();
  frames[backIndex] = getPublishedFrame();

  pipeline.load(in);
  pipeline.beginScanline(state.scanlineCtx, frames[backIndex]);
}
