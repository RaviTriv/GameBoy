# PPU

## Overview
The PPU is responsible for reading and processing data from memory (vram) to render Gameboy graphics.

It is a state machine, oam -> drawing -> hblank - vblank


### Components
- LCD Screen which is 160x144 pixels
  - State
    - LCDC union, LCD Control what elements and displayed and how
      - 
    - LCDS union, LCD Status
    - LY, current horizontal line, ranges from 0 - 153
    - LYC, compare against LY and set STAT when it is equal
    - DMA, writes to ppu oam directly, typically cpu would have to handle this which would be way slower <- ELABORATE ON THIS
    - palletes, array of background and object palletes

- Tile, 8 * 8 pixels
- 2BPP, 2 bits per pixel
- 2 Bytes make up a row of 8 pixels
- Layering System
  - Background: 256 * 256 pixels, only of which 160x144 pixels are visible at a time
    - Viewport determines what is visible when. Corresponds to registers `SCX` and `SCY`
  - Window: 256 * 256 pixels, controled by `WX` and `WY`
  - Sprites: 8 * 8 pixels, ocassionaly 8 * 16
- Memory
  - VRAM
  - OAM
- Video Buffer: calculated output for UI to read
- Scanline: Row of pixels (160 pixels), stored in `LY`
- `lineTicks` 

- State 
  - OAM_ENTRY, object attribute memory, stores Objects AKA sprites
    - x position
    - y position
    - tile index
    - flags
      - priority
      - y flip
      - x flip
      - DMG palette: shade of color
      - Bank: N/A
      - CGB Palette: N/A
  - lineTicks: tracks progress of ppu rendering of scan line
  - windowLine: stores y position within window
- Pipeline State
  - FetchStae: Current fetching mode
  - pixelFifo: circular buffer that holds pixels to render
  - lineX: horizontal position being processed
  - pushedCount: number of pixels pushed so far
  - fetchX: current x coordinate being fetched
  - bgwFetchData[]: temp background / window storage
  - fetchEntryData[]: temp object storage
  - mapY: y coordinate within background / window
  - mapX: x coordinate withing background / window
  - tileY: y position within current tile being fetched
  - fifo x: postion tracking for fifo operation
  - entryCount: number of sprites currently being processed

- Pipeline processing
  - update mapX, mapY, tileY, tileY is modulo 8, since each tile is 8 x 8, and multipled by 2 as theres 2 bytes per row
  - fetch on even clock cycles
  - push, check if we should display, push to video buffer, use correct index



### Modes
PPU cycles through several modes which 
OAM Scan (Mode 2) -> Drawing Pixels (Mode 3) -> Horizontal Bank (Mode 0) - Vertical Blank (Mode 1)

|Mode|Action|Duration|Accessible Video Memory|
|----|------|--------|------------------------|
|2|Search OAM memory for sprites to render |80 dots|VRAM, CGB palettes|
|3| Pixels are transferred to LCD | 172 - 289 dots | None|
|0| Adds time such that total cycles can reach 456. PPU pretty much paused | 376 - Mode3 Duration| VRAM, OAM, CGB palettes|
|1||||

#### OAM Mode
- Load sprites to render on the line, runs for 80 ticks
- Sets to drawing Mode once its hits 80 ticks

#### Drawing Mode


#### Hblank Mode
Occurs once a scaneline has finished being rendered.
During this time, CPU can acess VRAM and rest of system can sync up.

#### Vblank Mode
Similar to Hblank CPU can acess all memory and rest of systes can synce. This occurs once all scanlines have been written

### Pipeline

- Two FIFO Registers, each which can hold 8 pixels
  - Background 
  - OAM
