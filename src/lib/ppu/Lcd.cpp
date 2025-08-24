#include "../../../include/Lcd.h"
#include "../../../include/Logger.h"

LCD::LCD()
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