#include "Pipeline.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <optional>
#include <ostream>
#include <stdexcept>

#include "Common.h"
#include "Dma.h"
#include "ScanlineContext.h"
#include "Serializable.h"

namespace {
constexpr uint8_t MODE3_START_DELAY = 8;
constexpr uint8_t OBJ_COLOR_MASK = 0x3;
constexpr uint8_t OBJ_PALETTE = 0x4;
constexpr uint8_t OBJ_PRIORITY = 0x8;
constexpr uint8_t OAM_ENTRY_BYTES = 4;
constexpr uint8_t OAM_TILE_OFFSET = 2;
constexpr uint8_t OAM_FLAGS_OFFSET = 3;
constexpr uint8_t FLAG_DMG_PALETTE = 0x10;
constexpr uint8_t FLAG_X_FLIP = 0x20;
constexpr uint8_t FLAG_Y_FLIP = 0x40;
constexpr uint8_t FLAG_BG_PRIORITY = 0x80;
constexpr uint8_t MAP_MASK = 0x1F;
constexpr std::array<uint32_t, 4> DMG_COLORS = {0xFFFFFFFF, 0xFFAAAAAA,
                                                0xFF555555, 0xFF000000};

uint32_t paletteColor(uint8_t palette, uint8_t color) {
  return DMG_COLORS[(palette >> (color * 2)) & 3];
}
}  // namespace

Pipeline::Pipeline(VramView vram) : vram_(vram) {}

void Pipeline::beginScanline(const ScanlineContext& scanlineCtx,
                             FrameView target) {
  ctx = &scanlineCtx;
  target_ = target;
}

uint8_t Pipeline::readVram(uint16_t address) const {
  return vram_[address - VRAM_START];
}

bool Pipeline::previewFinishes(int dots) const {
  Pipeline preview = *this;
  preview.preview_ = true;
  for (int i = 0; i < dots && !preview.state.lineDone; i++) {
    preview.process();
  }
  return preview.state.lineDone;
}

bool Pipeline::latchRegisters() {
  uint8_t windowX = state.windowXHistory.back();
  for (std::size_t i = state.windowXHistory.size() - 1; i > 0; i--) {
    state.windowXHistory[i] = state.windowXHistory[i - 1];
  }
  state.windowXHistory[0] = ctx->windowX;
  bool windowXChanged = windowX != state.windowX;
  state.windowX = windowX;

  state.bgPalette = ctx->bgPalette | state.lateBgPalette;
  state.obj0Palette = ctx->obj0Palette | state.lateObj0Palette;
  state.obj1Palette = ctx->obj1Palette | state.lateObj1Palette;
  state.lateBgPalette = ctx->bgPalette;
  state.lateObj0Palette = ctx->obj0Palette;
  state.lateObj1Palette = ctx->obj1Palette;

  state.windowEnabled = state.lateWindowEnabled;
  state.bgEnabled = state.lateBgEnabled;
  state.objEnabled = state.lateObjEnabled;
  state.lateWindowEnabled = ctx->windowEnabled;
  state.lateBgEnabled = ctx->bgWindowEnabled;
  state.lateObjEnabled = ctx->objEnabled;
  return windowXChanged;
}

void Pipeline::process() {
  if (state.lineDone) {
    return;
  }

  bool windowXChanged = latchRegisters();

  if (state.startDelay > 0) {
    state.startDelay--;
    return;
  }

  if (state.objState != OBJ_STATE::IDLE) {
    if (ctx->objEnabled) {
      stepSpriteFetch();
      return;
    }
    state.objState = OBJ_STATE::IDLE;
  }

  if (!state.stalled && state.wyTriggered) {
    if (checkWindowTrigger(windowXChanged)) {
      state.stalled = true;
      return;
    }
  }
  state.stalled = false;
  if (state.windowFetching && !ctx->windowEnabled) {
    state.insertionGlitchDisabled = true;
  }

  if (matchSprite()) {
    state.objState = OBJ_STATE::WAIT;
    stepSpriteFetch();
    return;
  }

  renderPixel();
  advanceFetcher();

  if (state.lineX == XRES) {
    state.lineDone = true;
  }
}

bool Pipeline::checkWindowTrigger(bool windowXChanged) {
  uint8_t wx = state.windowX;
  bool fineScroll = (ctx->scrollX & 7) != 0;

  if (!state.wxTriggered && state.windowEnabled) {
    bool activate = false;
    if (wx == 0) {
      activate = state.lineX == -7 || (state.lineX == -16 && fineScroll) ||
                 (state.lineX >= -15 && state.lineX <= -8);
    } else if (wx < MAX_WINDOW_X) {
      activate =
          wx == static_cast<uint8_t>(state.lineX + 7) ||
          (wx == static_cast<uint8_t>(state.lineX + 6) && !windowXChanged);
    }

    if (activate) {
      state.windowLine++;
      state.windowTileX = 0;
      state.bgCount = 0;
      state.wxTriggered = true;
      state.fetchState = FETCH_STATE::TILE0;
      state.windowFetching = true;
      if (wx == 0 && fineScroll) {
        return true;
      }
    } else if (wx == MAX_WINDOW_X &&
               wx == static_cast<uint8_t>(state.lineX + 7)) {
      state.windowLine++;
    }
  }

  if (wx == static_cast<uint8_t>(state.lineX + 7) && state.wxTriggered &&
      !state.windowFetching && state.fetchState == FETCH_STATE::TILE0 &&
      state.bgCount == PIXEL_TILE_DIMENSION) {
    state.insertBgPixel = true;
  }
  return false;
}

uint8_t Pipeline::spriteMatchX() const {
  if (state.lineX < -8) {
    return 0;
  }
  return static_cast<uint8_t>(state.lineX + 8);
}

bool Pipeline::matchSprite() {
  uint8_t matchX = spriteMatchX();
  const auto& sprites = ctx->sprites;
  while (state.nextSprite < sprites.size() &&
         sprites[state.nextSprite].x < matchX) {
    state.nextSprite++;
  }
  return ctx->objEnabled && state.nextSprite < sprites.size() &&
         sprites[state.nextSprite].x == matchX;
}

void Pipeline::stepSpriteFetch() {
  switch (state.objState) {
    case OBJ_STATE::WAIT: {
      bool ready =
          state.fetchState >= FETCH_STATE::DATA1_1 && state.bgCount > 0;
      advanceFetcher();
      if (ready) {
        state.objState = OBJ_STATE::ATTR0;
      }
      break;
    }
    case OBJ_STATE::ATTR0:
      state.spriteTile = readSpriteOam(OAM_TILE_OFFSET);
      state.spriteTileRead = true;
      state.spriteFlags = readSpriteOam(OAM_FLAGS_OFFSET);
      advanceFetcher();
      state.objState = OBJ_STATE::ATTR1;
      break;
    case OBJ_STATE::ATTR1:
      state.objState = OBJ_STATE::DATA0_0;
      break;
    case OBJ_STATE::DATA0_0:
      state.spriteLow = readVram(spriteDataAddress());
      state.objState = OBJ_STATE::DATA0_1;
      break;
    case OBJ_STATE::DATA0_1:
      state.objState = OBJ_STATE::DATA1;
      break;
    case OBJ_STATE::DATA1:
      state.spriteHigh = readVram(spriteDataAddress() + 1);
      overlaySpriteRow();
      state.nextSprite++;
      state.objState = OBJ_STATE::IDLE;
      state.stalled = true;
      break;
    case OBJ_STATE::IDLE:
    default:
      break;
  }
}

std::optional<uint8_t> Pipeline::takeSpriteTileRead() {
  if (!state.spriteTileRead) {
    return std::nullopt;
  }
  state.spriteTileRead = false;
  return state.spriteTile;
}

uint8_t Pipeline::readSpriteOam(uint8_t offset) const {
  const auto address = static_cast<uint8_t>(
      (ctx->sprites[state.nextSprite].index * OAM_ENTRY_BYTES) + offset);
  return ctx->oam[ctx->dma->ppuOamAddress(address)];
}

void Pipeline::overlaySpriteRow() {
  uint8_t flags = (state.spriteFlags & FLAG_DMG_PALETTE) != 0 ? OBJ_PALETTE : 0;
  if ((state.spriteFlags & FLAG_BG_PRIORITY) != 0) {
    flags |= OBJ_PRIORITY;
  }

  for (int i = 0; i < PIXEL_TILE_DIMENSION; i++) {
    auto slot = static_cast<std::size_t>((state.objHead + i) & 7);
    if (i >= state.objCount) {
      state.objFifo[slot] = 0;
    }
    if ((state.objFifo[slot] & OBJ_COLOR_MASK) != 0) {
      continue;
    }
    int bit = (state.spriteFlags & FLAG_X_FLIP) != 0 ? i : 7 - i;
    auto color = static_cast<uint8_t>(((state.spriteLow >> bit) & 1) |
                                      (((state.spriteHigh >> bit) & 1) << 1));
    if (color != 0) {
      state.objFifo[slot] = color | flags;
    }
  }
  state.objCount = PIXEL_TILE_DIMENSION;
}

void Pipeline::advanceFetcher() {
  switch (state.fetchState) {
    case FETCH_STATE::TILE0:
      if (!ctx->windowEnabled) {
        state.wxTriggered = false;
      }
      state.fetchAddress = tileMapAddress();
      state.fetchState = FETCH_STATE::TILE1;
      break;
    case FETCH_STATE::TILE1:
      state.tileIndex = readVram(state.fetchAddress);
      state.fetchState = FETCH_STATE::DATA0_0;
      break;
    case FETCH_STATE::DATA0_0:
      state.fetchAddress = tileDataAddress();
      state.fetchState = FETCH_STATE::DATA0_1;
      break;
    case FETCH_STATE::DATA0_1:
      state.tileLow = readVram(state.fetchAddress);
      state.fetchState = FETCH_STATE::DATA1_0;
      break;
    case FETCH_STATE::DATA1_0:
      state.fetchAddress = tileDataAddress() + 1;
      state.fetchState = FETCH_STATE::DATA1_1;
      break;
    case FETCH_STATE::DATA1_1:
      state.tileHigh = readVram(state.fetchAddress);
      if (state.wxTriggered) {
        state.windowTileX = (state.windowTileX + 1) & MAP_MASK;
      }
      state.fetchState = FETCH_STATE::PUSH;
      [[fallthrough]];
    case FETCH_STATE::PUSH:
      if (state.bgCount > 0) {
        break;
      }
      if (state.wyTriggered && !ctx->windowEnabled &&
          !state.insertionGlitchDisabled) {
        auto position = static_cast<uint8_t>(state.lineX + 7);
        if (position > MAX_WINDOW_X + 1) {
          position = 0;
        }
        if (state.windowX == position) {
          state.bgLow = 0;
          state.bgHigh = 0;
          state.bgCount = 1;
          break;
        }
      }
      pushBgRow();
      state.fetchState = FETCH_STATE::TILE0;
      break;
    default:
      throw std::runtime_error("Trying to process unknown fetch state");
  }
}

void Pipeline::pushBgRow() {
  state.bgLow = state.tileLow;
  state.bgHigh = state.tileHigh;
  state.bgCount = PIXEL_TILE_DIMENSION;
}

void Pipeline::renderPixel() {
  const auto& sprites = ctx->sprites;
  if (ctx->objEnabled && state.nextSprite < sprites.size() &&
      sprites[state.nextSprite].x == 0) {
    return;
  }

  if (state.bgCount == 0) {
    return;
  }

  uint8_t bg = 0;
  if (state.insertBgPixel) {
    state.insertBgPixel = false;
  } else {
    bg = static_cast<uint8_t>(((state.bgHigh >> 6) & 2) | (state.bgLow >> 7));
    state.bgLow = static_cast<uint8_t>(state.bgLow << 1);
    state.bgHigh = static_cast<uint8_t>(state.bgHigh << 1);
    state.bgCount--;
  }

  uint8_t obj = 0;
  if (state.objCount > 0) {
    obj = state.objFifo[state.objHead];
    state.objHead = (state.objHead + 1) & 7;
    state.objCount--;
  }

  if (state.lineX < -8) {
    int fineScroll = ctx->scrollX & 7;
    if ((state.lineX & 7) == fineScroll) {
      state.lineX = -8;
    } else if (state.windowFetching && (state.lineX & 7) == 6 &&
               fineScroll == 7) {
      state.lineX = -8;
    } else if (state.lineX == -9) {
      state.lineX = -16;
      return;
    }
  }

  state.windowFetching = false;

  if (state.lineX < 0 || state.lineX >= XRES) {
    state.lineX++;
    return;
  }

  if (state.lineX == 0) {
    state.bgEnabled = ctx->bgWindowEnabled;
    state.objEnabled = ctx->objEnabled;
    state.bgPalette = ctx->bgPalette;
    state.obj0Palette = ctx->obj0Palette;
    state.obj1Palette = ctx->obj1Palette;
  }

  uint8_t bgColor = state.bgEnabled ? bg : 0;
  uint32_t color = paletteColor(state.bgPalette, bgColor);

  uint8_t objColor = obj & OBJ_COLOR_MASK;
  bool drawObj = objColor != 0 && state.objEnabled;
  if (drawObj && (obj & OBJ_PRIORITY) != 0 && bgColor != 0) {
    drawObj = false;
  }
  if (drawObj) {
    color = paletteColor(
        (obj & OBJ_PALETTE) != 0 ? state.obj1Palette : state.obj0Palette,
        objColor);
  }

  if (!preview_) {
    auto index = static_cast<std::size_t>(state.pushedCount) +
                 (static_cast<std::size_t>(ctx->ly) * XRES);
    contract_assert(index < target_.size());
    target_[index] = color;
  }
  state.pushedCount++;
  state.lineX++;
}

uint8_t Pipeline::fetcherY() const {
  if (state.wxTriggered) {
    return state.windowLine;
  }
  return static_cast<uint8_t>(ctx->ly + ctx->scrollY);
}

uint16_t Pipeline::tileMapAddress() const {
  uint16_t map = state.wxTriggered ? ctx->winMapArea : ctx->bgMapArea;
  int x = 0;
  if (state.wxTriggered) {
    x = state.windowTileX;
  } else if (state.lineX < -8) {
    x = ctx->scrollX >> 3;
  } else {
    x = ((ctx->scrollX + state.lineX + 8) / PIXEL_TILE_DIMENSION) & MAP_MASK;
  }
  int y = fetcherY() / PIXEL_TILE_DIMENSION;
  return static_cast<uint16_t>(map + x + (y * BACKGROUND_MAP_DIMENSION));
}

uint16_t Pipeline::tileDataAddress() const {
  int row = (fetcherY() & 7) * 2;
  if (ctx->bgWinDataArea == 0x8000) {
    return static_cast<uint16_t>(0x8000 + (state.tileIndex * 16) + row);
  }
  return static_cast<uint16_t>(
      0x9000 + (static_cast<int8_t>(state.tileIndex) * 16) + row);
}

uint16_t Pipeline::spriteDataAddress() const {
  const auto& sprite = ctx->sprites[state.nextSprite];
  bool tall = ctx->objHeight == 16;
  int mask = tall ? 0xF : 0x7;
  int row = (ctx->ly + 16 - sprite.y) & mask;
  if ((state.spriteFlags & FLAG_Y_FLIP) != 0) {
    row ^= mask;
  }
  int tile = tall ? (state.spriteTile & 0xFE) : state.spriteTile;
  return static_cast<uint16_t>(0x8000 + (tile * 16) + (row * 2));
}

void Pipeline::checkWindowY(uint8_t ly, uint8_t windowY, bool windowEnabled) {
  if (windowEnabled && windowY == ly) {
    state.wyTriggered = true;
  }
}

void Pipeline::reset() { state.bgCount = 0; }

void Pipeline::newFrame() {
  state.wyTriggered = false;
  state.windowLine = 0xFF;
}

void Pipeline::oamReset() {
  state.fetchState = FETCH_STATE::TILE0;
  state.objState = OBJ_STATE::IDLE;
  state.lineX = -16;
  state.pushedCount = 0;
  state.startDelay = MODE3_START_DELAY;
  state.nextSprite = 0;
  state.objHead = 0;
  state.objCount = 0;
  state.windowXHistory.fill(ctx->windowX);
  state.windowX = ctx->windowX;
  state.lateWindowEnabled = ctx->windowEnabled;
  state.lateBgEnabled = ctx->bgWindowEnabled;
  state.lateObjEnabled = ctx->objEnabled;
  state.lateBgPalette = ctx->bgPalette;
  state.lateObj0Palette = ctx->obj0Palette;
  state.lateObj1Palette = ctx->obj1Palette;
  state.wxTriggered = false;
  state.windowFetching = false;
  state.insertBgPixel = false;
  state.insertionGlitchDisabled = false;
  state.stalled = false;
  state.lineDone = false;
  state.bgLow = 0;
  state.bgHigh = 0;
  state.bgCount = PIXEL_TILE_DIMENSION;
}

void Pipeline::save(std::ostream& out) const { serial::saveFields(out, *this); }

void Pipeline::load(std::istream& in) {
  serial::loadFields(in, *this);
  if (state.pushedCount > XRES || state.bgCount > PIXEL_TILE_DIMENSION ||
      state.objCount > PIXEL_TILE_DIMENSION ||
      state.objHead >= PIXEL_TILE_DIMENSION ||
      state.nextSprite > MAX_LINE_SPRITES) {
    throw std::runtime_error("Invalid pipeline state in save file");
  }
}
