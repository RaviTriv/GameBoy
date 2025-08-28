#include "../../../include/Lcd.h"
#include "../../../include/Dma.h"
#include "../../../include/Logger.h"

LCD::LCD(std::shared_ptr<DMA> dma) : dma(dma)
{
  state.lcdc = 0x91;
  state.scrollX = 0;
  state.scrollY = 0;
  state.ly = 0;
  state.lyCompare = 0;
  state.windowX = 0;
  state.windowY = 0;
  for (int i = 0; i < 4; i++)
  {
    state.bgColors[i] = defaultColors[i];
    state.ob1Colors[i] = defaultColors[i];
    state.ob2Colors[i] = defaultColors[i];
  }
}

uint8_t LCD::read(uint16_t address)
{
  uint8_t offset = (address - 0xFF40);
  uint8_t *p = (uint8_t *)&state;

  return p[offset];
}

void LCD::write(uint16_t address, uint8_t value)
{
  uint8_t offset = (address - 0xFF40);
  uint8_t *p = (uint8_t *)&state;
  p[offset] = value;

  if (offset == 6)
  {
    dma->start(value);
  }

  if (address == 0xFF47)
  {
    updatePalettes(value, 0);
  }
  else if (address == 0xFF48)
  {
    updatePalettes(value & 0b11111100, 1);
  }
  else if (address == 0xFF49)
  {
    updatePalettes(value & 0b11111100, 2);
  }
}

void LCD::updatePalettes(uint8_t paletteData, uint8_t pal)
{
  uint32_t *pColors = state.bgColors.data();

  switch (pal)
  {
  case 1:
    pColors = state.ob1Colors.data();
    break;
  case 2:
    pColors = state.ob2Colors.data();
    break;
  }

  pColors[0] = defaultColors.at(paletteData & 0b11);
  pColors[1] = defaultColors.at((paletteData >> 2) & 0b11);
  pColors[2] = defaultColors.at((paletteData >> 4) & 0b11);
  pColors[3] = defaultColors.at((paletteData >> 6) & 0b11);
}

bool LCD::isLcdStatIntEnabled(uint8_t source)
{
  return (state.lcds & source) != 0;
}

bool LCD::getBit(uint8_t value, int bit) const
{
  return (value & (1 << bit)) != 0;
}

void LCD::setBit(uint8_t &value, int bit, bool set)
{
  if (set)
  {
    value |= (1 << bit);
  }
  else
  {
    value &= ~(1 << bit);
  }
}

bool LCD::isBgWindowEnabled()
{
  return getBit(state.lcdc, 0);
}

bool LCD::isObjEnabled()
{
  return getBit(state.lcdc, 1);
}

uint8_t LCD::getObjHeight()
{
  return getBit(state.lcdc, 2) ? 16 : 8;
}

uint16_t LCD::getBgMapArea() const
{
  return getBit(state.lcdc, 3) ? 0x9C00 : 0x9800;
}

uint16_t LCD::getBgWindowDataArea() const
{
  return getBit(state.lcdc, 4) ? 0x8000 : 0x8800;
}

bool LCD::isWindowEnabled()
{
  return getBit(state.lcdc, 5);
}

uint16_t LCD::getWindowMapArea() const
{
  return getBit(state.lcdc, 6) ? 0x9C00 : 0x9800;
}

int LCD::getLcdMode() const
{
  return static_cast<MODE>(state.lcds & 0b11);
}

void LCD::setLcdMode(MODE mode)
{
  state.lcds &= ~0b11;
  state.lcds |= static_cast<uint8_t>(mode);
}

bool LCD::isLycFlag()
{
  return getBit(state.lcds, 2);
}

void LCD::setLycFlag(bool value)
{
  setBit(state.lcds, 2, value);
}

uint8_t LCD::getLy()
{
  return state.ly;
}

void LCD::incrementLy()
{
  state.ly++;
}

uint8_t LCD::getLyCompare()
{
  return state.lyCompare;
}

void LCD::setLy(uint8_t value)
{
  state.ly = value;
}

uint8_t LCD::getWinX()
{
  return state.windowX;
}

uint8_t LCD::getWinY()
{
  return state.windowY;
}

uint8_t LCD::getScrollX()
{
  return state.scrollX;
}

uint8_t LCD::getScrollY()
{
  return state.scrollY;
}

uint32_t LCD::getBgColor(uint8_t idx)
{
  return state.bgColors[idx];
};

uint32_t LCD::getOb1Colors(uint8_t idx)
{
  return state.ob1Colors[idx];
}

uint32_t LCD::getOb2Colors(uint8_t idx)
{
  return state.ob2Colors[idx];
}