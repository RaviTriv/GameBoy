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
  state.palettes[PaletteType::BGP].palette = 0xFC;
  state.palettes[PaletteType::OBP0].palette = 0xFF;
  state.palettes[PaletteType::OBP1].palette = 0xFF;
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
  switch (address)
  {
  case 0xFF40:
    return state.lcdc;
  case 0xFF41:
    return state.lcds;
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
    Logger::GetLogger()->error("LCD read from invalid address: 0x{:04X}", address);
    return 0xFF;
  }
}

void LCD::write(uint16_t address, uint8_t value)
{
  switch (address)
  {
  case 0xFF40:
    state.lcdc = value;
    break;
  case 0xFF41:
    state.lcds = value & 0xFC;
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
    break;
  case 0xFF46:
    dma->start(value);
    break;
  case 0xFF47:
    updatePalettes(PaletteType::BGP, value);
    break;
  case 0xFF48:
    updatePalettes(PaletteType::OBP0, value & 0b11111100);
    break;
  case 0xFF49:
    updatePalettes(PaletteType::OBP1, value & 0b11111100);
    break;
  case 0xFF4A:
    state.windowY = value;
    break;
  case 0xFF4B:
    state.windowX = value;
    break;
  default:
    Logger::GetLogger()->error("LCD write to invalid address: 0x{:04X}", address);
    break;
  }
}

void LCD::updatePalettes(PaletteType type, uint8_t value)
{
  switch (type)
  {
  case BGP:
    state.bgColors[0] = defaultColors.at(value & 0b11);
    state.bgColors[1] = defaultColors.at((value >> 2) & 0b11);
    state.bgColors[2] = defaultColors.at((value >> 4) & 0b11);
    state.bgColors[3] = defaultColors.at((value >> 6) & 0b11);
    break;
  case OBP0:
    state.ob1Colors[0] = defaultColors.at(value & 0b11);
    state.ob1Colors[1] = defaultColors.at((value >> 2) & 0b11);
    state.ob1Colors[2] = defaultColors.at((value >> 4) & 0b11);
    state.ob1Colors[3] = defaultColors.at((value >> 6) & 0b11);
    break;
  case OBP1:
    state.ob2Colors[0] = defaultColors.at(value & 0b11);
    state.ob2Colors[1] = defaultColors.at((value >> 2) & 0b11);
    state.ob2Colors[2] = defaultColors.at((value >> 4) & 0b11);
    state.ob2Colors[3] = defaultColors.at((value >> 6) & 0b11);
    break;
  default:
    Logger::GetLogger()->error("Invalid palette type: {}", static_cast<int>(type));
    return;
  }
}

bool LCD::isLcdStatIntEnabled(uint8_t source)
{
  return (state.lcds & source) != 0;
}

uint16_t LCD::getBgMapArea() const
{
  return state.lcdcBits.bgTileMap ? 0x9C00 : 0x9800;
}

uint16_t LCD::getBgWindowDataArea() const
{
  return state.lcdcBits.bgWindowTiles ? 0x8000 : 0x8800;
}

uint16_t LCD::getWindowMapArea() const
{
  return state.lcdcBits.windowTileMap ? 0x9C00 : 0x9800;
}