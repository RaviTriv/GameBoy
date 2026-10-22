#include "Ppu.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <utility>

#include "Common.h"
#include "Interrupts.h"
#include "Lcd.h"
#include "Logger.h"
#include "OamTypes.h"
#include "Serializable.h"

PPU::PPU(Interrupts& interrupts, LCD& lcd)
    : interrupts_(interrupts), lcd_(lcd), pipeline(state.vram) {
  state.lineTicks = BOOT_LINE_TICKS;
  setMode(LCD::MODE::VBLANK);
  lcd_.setLycFlag(lcd_.getLy() == lcd_.getLyCompare());
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
      setMode(LCD::MODE::HBLANK);
      pipeline.reset();
      state.lineTicks = 0;
      state.windowLine = 0;
      frames[backIndex].fill(0xFFFFFFFF);
      finishFrame();
      return;
    case LCD::TURNED_ON:
      lcd_.setLy(0);
      startLine();
      state.skipOamScan = true;
      state.lineTicks = LCD_ON_LINE_TICKS;
      state.windowLine = 0;
      state.lineSprites.clear();
      loadLineSprites();
      break;
    case LCD::NO_TRANSITION:
      break;
  }

  if (!lcd_.isLcdEnabled()) {
    return;
  }

  state.lineTicks++;
  switch (state.mode) {
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
      Logger::GetLogger()->error("Unknown PPU mode: {}",
                                 static_cast<int>(state.mode));
      break;
  }
}

void PPU::setMode(LCD::MODE mode) {
  state.mode = mode;
  lcd_.setLcdMode(mode);
}

void PPU::startLine() {
  state.mode = LCD::MODE::OAM;
  state.skipOamScan = false;
  lcd_.setLcdMode(LCD::MODE::HBLANK);
}

bool PPU::isOamBlocked() const {
  if (!lcd_.isLcdEnabled()) {
    return false;
  }
  if (state.skipOamScan) {
    return lcd_.getLcdMode() == LCD::MODE::DRAWING;
  }
  return state.mode == LCD::MODE::OAM || state.mode == LCD::MODE::DRAWING;
}

bool PPU::isVramBlocked() const {
  if (!lcd_.isLcdEnabled()) {
    return false;
  }
  if (state.skipOamScan) {
    return lcd_.getLcdMode() == LCD::MODE::DRAWING;
  }
  return state.mode == LCD::MODE::DRAWING;
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
  lcd_.setLycFlag(false);
}

void PPU::compareLyc(uint8_t ly) {
  if (ly == lcd_.getLyCompare()) {
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

    if (state.lineSprites.size() == state.lineSprites.capacity()) {
      break;
    }

    if (entry.y <= curY + 16 && entry.y + spriteHeight > curY + 16) {
      auto& sprites = state.lineSprites;
      sprites.unchecked_push_back(entry);
      for (auto pos = sprites.size() - 1;
           pos > 0 && sprites[pos - 1].x > entry.x; pos--) {
        std::swap(sprites[pos], sprites[pos - 1]);
      }
    }
  }
}

uint32_t PPU::calculateSpritePenalty() const {
  if (!lcd_.isObjEnabled()) {
    return 0;
  }
  uint32_t penalty = 0;
  int lastTile = -1;
  int fineScrollX = lcd_.getScrollX() % 8;
  for (const auto& sprite : state.lineSprites) {
    if (sprite.x >= XRES + 8) {
      continue;
    }
    int pixel = sprite.x == 0 ? 0 : sprite.x + fineScrollX;
    int tile = pixel / 8;
    if (tile != lastTile) {
      lastTile = tile;
      penalty += static_cast<uint32_t>(std::max(0, 5 - (pixel % 8)));
    }
    penalty += SPRITE_FETCH_PENALTY;
  }
  return penalty;
}

uint32_t PPU::calculateWindowPenalty() const {
  if (lcd_.isWindowEnabled() && lcd_.getWindowX() < MAX_WINDOW_X &&
      lcd_.getLy() >= lcd_.getWindowY()) {
    return lcd_.getWindowX() == 0 ? WINDOW_PENALTY + 1 : WINDOW_PENALTY;
  }
  return 0;
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
  state.scanlineCtx.sprites = state.lineSprites;
  state.scanlineCtx.lineTicks = &state.lineTicks;
}

void PPU::oamMode() {
  if (state.lineTicks == 1) {
    state.lineSprites.clear();
    loadLineSprites();
  }

  if (state.lineTicks == STAT_MODE_DELAY) {
    if (lcd_.getLy() == 0) {
      requestStatInterrupt(LCD::LCDS_SRC::S_OAM);
    }
    lcd_.setLcdMode(LCD::MODE::OAM);
    if (lcd_.getLy() != 0) {
      compareLyc(lcd_.getLy());
    }
  }

  if (state.lineTicks >= 80 + (state.skipOamScan ? LCD_ON_MODE3_DELAY : 0)) {
    state.mode = LCD::MODE::DRAWING;
    state.mode3Penalty = calculateSpritePenalty() + calculateWindowPenalty();

    buildScanlineContext();
    pipeline.beginScanline(state.scanlineCtx, frames[backIndex]);
    pipeline.oamReset();
    pipeline.process();
  }
}

void PPU::drawingMode() {
  if (state.lineTicks == 80 + STAT_MODE_DELAY) {
    lcd_.setLcdMode(LCD::MODE::DRAWING);
  }

  if (pipeline.getPushedCount() < XRES) {
    pipeline.process();
  }

  if (pipeline.getPushedCount() >= XRES) {
    if (state.mode3Penalty > 0) {
      state.mode3Penalty--;
      return;
    }

    if (lcd_.getLcdMode() != LCD::MODE::HBLANK) {
      lcd_.setLcdMode(LCD::MODE::HBLANK);
      return;
    }

    pipeline.reset();
    setMode(LCD::MODE::HBLANK);
    requestStatInterrupt(LCD::LCDS_SRC::S_HBLANK);
  }
}

void PPU::hBlankMode() {
  if (state.lineTicks >= TICKS_PER_LINE) {
    incrementLY();

    if (lcd_.getLy() >= YRES) {
      state.mode = LCD::MODE::VBLANK;
      lcd_.setLcdMode(LCD::MODE::HBLANK);
      finishFrame();
    } else {
      requestStatInterrupt(LCD::LCDS_SRC::S_OAM);
      startLine();
    }

    state.lineTicks = 0;
  }
}

void PPU::requestStatInterrupt(LCD::LCDS_SRC source) {
  if (!lcd_.isLcdStatIntEnabled(static_cast<uint8_t>(source))) {
    return;
  }
  if ((state.lineTicks + 3) % CLOCK_CYCLES >= 2) {
    interrupts_.requestLate(InterruptType::LCD_STAT);
  } else {
    interrupts_.request(InterruptType::LCD_STAT);
  }
}

void PPU::enterVBlank() {
  lcd_.setLcdMode(LCD::MODE::VBLANK);
  interrupts_.request(InterruptType::VBLANK);

  if (lcd_.isLcdStatIntEnabled(static_cast<uint8_t>(LCD::LCDS_SRC::S_VBLANK))) {
    interrupts_.request(InterruptType::LCD_STAT);
  }
}

void PPU::vBlankMode() {
  if (state.lineTicks == STAT_MODE_DELAY) {
    if (lcd_.getLy() == YRES) {
      enterVBlank();
    }
    uint8_t ly = lcd_.getLy();
    if (ly == LINES_PER_FRAME - 1) {
      lcd_.setLy(0);
    }
    compareLyc(ly);
  } else if (lcd_.getLy() == 0) {
    if (state.lineTicks == LINE_153_LYC_CLEAR_TICKS) {
      lcd_.setLycFlag(false);
    } else if (state.lineTicks == LINE_153_LYC_COMPARE_TICKS) {
      compareLyc(0);
    }
  }

  if (state.lineTicks >= TICKS_PER_LINE) {
    if (lcd_.getLy() == 0) {
      startLine();
      state.windowLine = 0;
    } else {
      incrementLY();
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
  serial::saveFields(out, *this);
  serial::write(out, getPublishedFrame());
}

void PPU::load(std::istream& in) {
  serial::loadFields(in, *this);
  if (state.scanlineCtx.ly >= YRES) {
    throw std::runtime_error("Invalid PPU scanline in save file");
  }
  state.scanlineCtx.sprites = state.lineSprites;
  state.scanlineCtx.lineTicks = &state.lineTicks;

  serial::read(in, frames[backIndex]);
  publishFrame();
  frames[backIndex] = getPublishedFrame();

  pipeline.beginScanline(state.scanlineCtx, frames[backIndex]);
}
