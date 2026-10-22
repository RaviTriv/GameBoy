#include "Lcd.h"

#include <array>
#include <cstdint>
#include <istream>
#include <ostream>
#include <utility>

#include "Dma.h"
#include "Logger.h"
#include "Serializable.h"

LCD::LCD(DMA& dma) : dma_(dma) {
  state.lcdc = 0x91;
  state.scrollX = 0;
  state.scrollY = 0;
  state.ly = 0;
  state.lyCompare = 0;
  state.dma = 0xFF;
  state.palettes[PaletteType::BGP].palette = 0xFC;
  state.palettes[PaletteType::OBP0].palette = 0xFF;
  state.palettes[PaletteType::OBP1].palette = 0xFF;
  state.windowX = 0;
  state.windowY = 0;
}

uint8_t LCD::read(uint16_t address) const {
  switch (address) {
    case 0xFF40:
      return state.lcdc;
    case 0xFF41:
      return state.lcds | 0x80;
    case 0xFF42:
      return state.scrollY;
    case 0xFF43:
      return state.scrollX;
    case 0xFF44:
      return state.ly;
    case 0xFF45:
      return state.lyCompare;
    case 0xFF46:
      return state.dma;
    case 0xFF47:
      return state.palettes[PaletteType::BGP].palette;
    case 0xFF48:
      return state.palettes[PaletteType::OBP0].palette;
    case 0xFF49:
      return state.palettes[PaletteType::OBP1].palette;
    case 0xFF4A:
      return state.windowY;
    case 0xFF4B:
      return state.windowX;
    default:
      Logger::GetLogger()->error("LCD read from invalid address: 0x{:04X}",
                                 address);
      return 0xFF;
  }
}

void LCD::write(uint16_t address, uint8_t value) {
  switch (address) {
    case 0xFF40:
      state.lcdc = value;
      break;
    case 0xFF41:
      state.lcds = (state.lcds & 0x07) | (value & 0x78);
      statWrites |= STAT_WRITTEN;
      break;
    case 0xFF42:
      state.scrollY = value;
      break;
    case 0xFF43:
      state.scrollX = value;
      break;
    case 0xFF44:
      break;
    case 0xFF45:
      state.lyCompare = value;
      statWrites |= LYC_WRITTEN;
      break;
    case 0xFF46:
      state.dma = value;
      dma_.start(value);
      break;
    case 0xFF47:
      state.palettes[PaletteType::BGP].palette = value;
      break;
    case 0xFF48:
      state.palettes[PaletteType::OBP0].palette = value & 0b11111100;
      break;
    case 0xFF49:
      state.palettes[PaletteType::OBP1].palette = value & 0b11111100;
      break;
    case 0xFF4A:
      state.windowY = value;
      break;
    case 0xFF4B:
      state.windowX = value;
      break;
    default:
      Logger::GetLogger()->error("LCD write to invalid address: 0x{:04X}",
                                 address);
      break;
  }
}

bool LCD::isLcdStatIntEnabled(uint8_t source) const {
  return (state.lcds & source) != 0;
}

bool LCD::getBit(uint8_t value, int bit) const {
  return (value & (1 << bit)) != 0;
}

void LCD::setBit(uint8_t& value, int bit, bool set) {
  if (set) {
    value |= static_cast<uint8_t>(1 << bit);
  } else {
    value &= static_cast<uint8_t>(~(1 << bit));
  }
}

bool LCD::isBgWindowEnabled() const { return getBit(state.lcdc, 0); }

bool LCD::isObjEnabled() const { return getBit(state.lcdc, 1); }

uint8_t LCD::getObjHeight() const { return getBit(state.lcdc, 2) ? 16 : 8; }

uint16_t LCD::getBgMapArea() const {
  return getBit(state.lcdc, 3) ? 0x9C00 : 0x9800;
}

uint16_t LCD::getBgWindowDataArea() const {
  return getBit(state.lcdc, 4) ? 0x8000 : 0x8800;
}

bool LCD::isWindowEnabled() const { return getBit(state.lcdc, 5); }

uint16_t LCD::getWindowMapArea() const {
  return getBit(state.lcdc, 6) ? 0x9C00 : 0x9800;
}

int LCD::getLcdMode() const { return static_cast<MODE>(state.lcds & 0b11); }

void LCD::setLcdMode(MODE mode) {
  state.lcds &= static_cast<uint8_t>(~0b11);
  state.lcds |= static_cast<uint8_t>(mode);
}

void LCD::setLycFlag(bool value) { setBit(state.lcds, 2, value); }

bool LCD::isLcdEnabled() const { return getBit(state.lcdc, 7); }

LCD::EnableTransition LCD::consumeEnableTransition() {
  bool enabled = isLcdEnabled();
  EnableTransition transition = NO_TRANSITION;
  if (enabled && !lcdWasEnabled) {
    transition = TURNED_ON;
  } else if (!enabled && lcdWasEnabled) {
    transition = TURNED_OFF;
  }
  lcdWasEnabled = enabled;
  return transition;
}

uint8_t LCD::consumeStatWrites() { return std::exchange(statWrites, NO_WRITE); }

void LCD::save(std::ostream& out) const { serial::saveFields(out, *this); }

void LCD::load(std::istream& in) { serial::loadFields(in, *this); }

uint8_t LCD::getLy() const { return state.ly; }

void LCD::setLy(uint8_t value) { state.ly = value; }

void LCD::incrementLy() { state.ly++; }

uint8_t LCD::getLyCompare() const { return state.lyCompare; }

uint8_t LCD::getScrollX() const { return state.scrollX; }

uint8_t LCD::getScrollY() const { return state.scrollY; }

uint8_t LCD::getWindowX() const { return state.windowX; }

uint8_t LCD::getWindowY() const { return state.windowY; }

uint8_t LCD::getPalette(PaletteType type) const {
  return state.palettes[type].palette;
}
