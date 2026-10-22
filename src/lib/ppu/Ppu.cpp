#include "Ppu.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <optional>
#include <ostream>
#include <span>
#include <utility>

#include "Common.h"
#include "Interrupts.h"
#include "Lcd.h"
#include "Logger.h"
#include "OamTypes.h"
#include "Serializable.h"

PPU::PPU(Interrupts& interrupts, LCD& lcd,
         std::span<const uint8_t, HEADER_LOGO_SIZE> bootLogo)
    : interrupts_(interrupts), lcd_(lcd), pipeline(state.vram) {
  state.lineTicks = BOOT_LINE_TICKS;
  setMode(LCD::MODE::VBLANK);
  updateStatLine(false);
  loadBootVram(bootLogo);
}

namespace {

constexpr std::size_t BOOT_LOGO_TILES = 0x0010;
constexpr std::size_t BOOT_MAP_ROW0 = 0x1904;
constexpr std::size_t BOOT_MAP_ROW1 = 0x1924;
constexpr std::size_t BOOT_MAP_REGISTERED = 0x1910;
constexpr uint8_t BOOT_TILES_PER_ROW = 12;
constexpr std::array<uint8_t, 8> BOOT_REGISTERED_TILE = {
    0x3C, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C};

}  // namespace

void PPU::loadBootVram(std::span<const uint8_t, HEADER_LOGO_SIZE> logo) {
  std::size_t addr = BOOT_LOGO_TILES;
  auto writeNibble = [&](uint8_t nibble) {
    uint8_t row = 0;
    for (int bit = 3; bit >= 0; bit--) {
      row = static_cast<uint8_t>((row << 2) | (((nibble >> bit) & 1) * 0b11));
    }
    state.vram[addr] = row;
    state.vram[addr + 2] = row;
    addr += 4;
  };
  for (uint8_t value : logo) {
    writeNibble(static_cast<uint8_t>(value >> 4));
    writeNibble(static_cast<uint8_t>(value & NIBBLE_MASK));
  }
  for (uint8_t row : BOOT_REGISTERED_TILE) {
    state.vram[addr] = row;
    addr += 2;
  }
  for (uint8_t i = 0; i < BOOT_TILES_PER_ROW; i++) {
    state.vram[BOOT_MAP_ROW0 + i] = static_cast<uint8_t>(i + 1);
    state.vram[BOOT_MAP_ROW1 + i] =
        static_cast<uint8_t>(i + 1 + BOOT_TILES_PER_ROW);
  }
  state.vram[BOOT_MAP_REGISTERED] = (2 * BOOT_TILES_PER_ROW) + 1;
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
  if (state.blankFrame) {
    frames[backIndex].fill(BLANK_PIXEL);
    state.blankFrame = false;
  }
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
  uint8_t statWrites = lcd_.consumeStatWrites();
  if (statWrites != LCD::NO_WRITE && lcd_.isLcdEnabled()) {
    if ((statWrites & LCD::STAT_WRITTEN) != 0) {
      updateStatLine(false, STAT_ALL_SOURCES);
    }
    updateStatLine(false);
  }

  switch (lcd_.consumeEnableTransition()) {
    case LCD::TURNED_OFF:
      lcd_.setLy(0);
      setMode(LCD::MODE::HBLANK);
      pipeline.reset();
      state.lineTicks = 0;
      pipeline.newFrame();
      frames[backIndex].fill(BLANK_PIXEL);
      finishFrame();
      return;
    case LCD::TURNED_ON:
      lcd_.setLy(0);
      startLine();
      state.statSource = 0;
      compareLyc(0);
      state.skipOamScan = true;
      state.blankFrame = true;
      state.lineTicks = LCD_ON_LINE_TICKS;
      pipeline.newFrame();
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
  if ((state.lineTicks % CLOCK_CYCLES) == 1 &&
      state.mode != LCD::MODE::VBLANK && !pipeline.isWindowYTriggered()) {
    pipeline.checkWindowY(lcd_.getLy(), lcd_.getWindowY(),
                          lcd_.isWindowEnabled());
  }
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

namespace {

constexpr std::size_t WORD_BYTES = 2;
constexpr std::size_t THIRD_WORD = 4;
constexpr std::size_t FIRST_SECONDARY_ROW = 4;

using Row = std::span<uint8_t, 8>;

Row rowAt(std::span<uint8_t> oam, std::size_t row) {
  return oam.subspan(row * Row::extent).first<Row::extent>();
}

void copyRow(Row from, Row to) { std::ranges::copy(from, to.begin()); }

void corruptWrite(Row cur, Row prev) {
  for (std::size_t i = 0; i < WORD_BYTES; i++) {
    const uint8_t a = cur[i];
    const uint8_t b = prev[i];
    const uint8_t c = prev[THIRD_WORD + i];
    cur[i] = static_cast<uint8_t>(((a ^ c) & (b ^ c)) ^ c);
  }
  std::ranges::copy(prev.subspan<WORD_BYTES>(),
                    cur.subspan<WORD_BYTES>().begin());
}

void corruptRead(Row cur, Row prev) {
  for (std::size_t i = 0; i < WORD_BYTES; i++) {
    const uint8_t a = cur[i];
    const uint8_t b = prev[i];
    const uint8_t c = prev[THIRD_WORD + i];
    cur[i] = static_cast<uint8_t>(b | (a & c));
  }
  std::ranges::copy(prev.subspan<WORD_BYTES>(),
                    cur.subspan<WORD_BYTES>().begin());
}

void corruptReadIdu(Row cur, Row prev, Row prev2) {
  for (std::size_t i = 0; i < WORD_BYTES; i++) {
    const uint8_t a = prev2[i];
    const uint8_t b = prev[i];
    const uint8_t c = cur[i];
    const uint8_t d = prev[THIRD_WORD + i];
    prev[i] = static_cast<uint8_t>((b & (a | c | d)) | (a & c & d));
  }
  copyRow(prev, cur);
  copyRow(prev, prev2);
}

}  // namespace

bool PPU::triggerOamBug(OamBugAccess access) {
  const auto row = oamScanRow();
  if (!row || *row == 0) {
    return false;
  }
  const std::span<uint8_t, OAM_BYTES> oam = oamBytes();
  const Row cur = rowAt(oam, *row);
  const Row prev = rowAt(oam, *row - 1);
  switch (access) {
    case OamBugAccess::Write:
      corruptWrite(cur, prev);
      break;
    case OamBugAccess::ReadIdu:
      if (*row >= FIRST_SECONDARY_ROW && *row < OAM_ROWS - 1) {
        corruptReadIdu(cur, prev, rowAt(oam, *row - 2));
      }
      corruptRead(cur, prev);
      break;
    case OamBugAccess::Read:
      corruptRead(cur, prev);
      break;
  }
  return true;
}

std::optional<std::size_t> PPU::oamScanRow() const {
  if (!lcd_.isLcdEnabled() || state.skipOamScan ||
      state.mode != LCD::MODE::OAM) {
    return std::nullopt;
  }
  return std::min<std::size_t>(
      state.lineTicks / static_cast<uint32_t>(CLOCK_CYCLES), OAM_ROWS - 1);
}

std::span<uint8_t, PPU::OAM_BYTES> PPU::oamBytes() {
  return std::span<uint8_t, OAM_BYTES>(
      reinterpret_cast<uint8_t*>(state.oamRam.data()), OAM_BYTES);
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
  lcd_.incrementLy();
  compareLyc(NO_LY);
}

void PPU::compareLyc(int16_t ly) {
  state.lyForComparison = ly;
  updateStatLine(false);
}

void PPU::updateStatLine(bool phased) {
  updateStatLine(phased, lcd_.read(0xFF41) & STAT_ALL_SOURCES);
}

void PPU::updateStatLine(bool phased, uint8_t enabled) {
  if (state.lyForComparison != NO_LY) {
    state.lycLine = state.lyForComparison == lcd_.getLyCompare();
    lcd_.setLycFlag(state.lycLine);
  } else {
    lcd_.setLycFlag(false);
  }
  bool line = (state.statSource & enabled) != 0 ||
              (state.lycLine && (enabled & LCD::LCDS_SRC::S_LYC) != 0);
  if (line && !state.statLine) {
    requestStat(phased);
  }
  state.statLine = line;
}

void PPU::setStatSource(uint8_t source, bool phased) {
  state.statSource = source;
  updateStatLine(phased);
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

void PPU::buildScanlineContext() {
  state.scanlineCtx.scrollX = lcd_.getScrollX();
  state.scanlineCtx.scrollY = lcd_.getScrollY();
  state.scanlineCtx.ly = lcd_.getLy();
  state.scanlineCtx.windowX = lcd_.getWindowX();
  state.scanlineCtx.windowY = lcd_.getWindowY();
  state.scanlineCtx.bgWindowEnabled = lcd_.isBgWindowEnabled();
  state.scanlineCtx.objEnabled = lcd_.isObjEnabled();
  state.scanlineCtx.windowEnabled = lcd_.isWindowEnabled();
  state.scanlineCtx.objHeight = lcd_.getObjHeight();
  state.scanlineCtx.bgMapArea = lcd_.getBgMapArea();
  state.scanlineCtx.bgWinDataArea = lcd_.getBgWindowDataArea();
  state.scanlineCtx.winMapArea = lcd_.getWindowMapArea();
  state.scanlineCtx.bgPalette = lcd_.getPalette(LCD::BGP);
  state.scanlineCtx.obj0Palette = lcd_.getPalette(LCD::OBP0);
  state.scanlineCtx.obj1Palette = lcd_.getPalette(LCD::OBP1);
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
      setStatSource(LCD::LCDS_SRC::S_OAM, true);
    }
    lcd_.setLcdMode(LCD::MODE::OAM);
    state.statSource = 0;
    compareLyc(lcd_.getLy());
  }

  if (state.lineTicks >= 80 + (state.skipOamScan ? LCD_ON_MODE3_DELAY : 0)) {
    state.mode = LCD::MODE::DRAWING;

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

  if ((state.lineTicks % CLOCK_CYCLES) == 1) {
    buildScanlineContext();
  }
  pipeline.process();

  if (!pipeline.finishesWithin(PIPELINE_LAG)) {
    return;
  }

  if (lcd_.getLcdMode() != LCD::MODE::HBLANK) {
    lcd_.setLcdMode(LCD::MODE::HBLANK);
    return;
  }

  setMode(LCD::MODE::HBLANK);
  setStatSource(LCD::LCDS_SRC::S_HBLANK, true);
}

void PPU::hBlankMode() {
  if (!pipeline.isLineDone()) {
    if ((state.lineTicks % CLOCK_CYCLES) == 1) {
      buildScanlineContext();
    }
    pipeline.process();
  }

  if (state.lineTicks >= TICKS_PER_LINE) {
    incrementLY();

    if (lcd_.getLy() >= YRES) {
      state.mode = LCD::MODE::VBLANK;
      lcd_.setLcdMode(LCD::MODE::HBLANK);
      if (!state.statLine && lcd_.isLcdStatIntEnabled(LCD::LCDS_SRC::S_OAM)) {
        requestStat(true);
      }
      finishFrame();
    } else {
      setStatSource(LCD::LCDS_SRC::S_OAM, true);
      startLine();
    }

    state.lineTicks = 0;
  }
}

void PPU::requestStat(bool phased) {
  if (phased && (state.lineTicks + 3) % CLOCK_CYCLES >= 2) {
    interrupts_.requestLate(InterruptType::LCD_STAT);
  } else {
    interrupts_.request(InterruptType::LCD_STAT);
  }
}

void PPU::enterVBlank() {
  lcd_.setLcdMode(LCD::MODE::VBLANK);
  interrupts_.request(InterruptType::VBLANK);
  if (!state.statLine && lcd_.isLcdStatIntEnabled(LCD::LCDS_SRC::S_OAM)) {
    requestStat(false);
  }
  setStatSource(LCD::LCDS_SRC::S_VBLANK, false);
}

void PPU::vBlankMode() {
  if (state.lineTicks == STAT_MODE_DELAY) {
    uint8_t ly = lcd_.getLy();
    if (ly == LINES_PER_FRAME - 1) {
      lcd_.setLy(0);
    }
    compareLyc(ly);
    if (ly == YRES) {
      enterVBlank();
    }
  } else if (lcd_.getLy() == 0) {
    if (state.lineTicks == LINE_153_LYC_CLEAR_TICKS) {
      compareLyc(NO_LY);
    } else if (state.lineTicks == LINE_153_LYC_COMPARE_TICKS) {
      compareLyc(0);
    }
  }

  if (state.lineTicks >= TICKS_PER_LINE) {
    if (lcd_.getLy() == 0) {
      startLine();
      pipeline.newFrame();
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
