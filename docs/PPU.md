# PPU
The PPU is responsible for reading and processing data from memory (vram) to render Gameboy graphics. 

## Display
Background
- 256 * 256 pixels
- scrollable
Window
- 256 * 256 pixels
- Overlays over the background.
- Fixed position via register, `wx` and `wy`
- Can toggle visibility, partial or full
- Used for things such as text dialog boxes in pokemon
Viewport
- 160 * 144 pixels, the full screen that is actually visible
Sprite
- 8 * 8 pixels, actual objects,
- appears on top of background typically
- 40 Total, with max 10 per line
Tile: Gameboy only processes in 8 * 8 pixel tiles

## Rendering
The gameboy renders pixels in whats is known as a scan line. It goes from left to right top to bottom.

## Modes
At the top level the following state machine exists
```                                                                         
  ┌─────┐       ┌─────────┐       ┌────────┐       ┌────────┐           
  │     │       │         │       │        │       │        │           
  │ OAM ├──────►│ Drawing ├──────►│ HBlank ├──────►│ VBlank │           
  │     │       │         │       │        │       │        │           
  └─────┘       └─────────┘       └────────┘       └────┬───┘           
     ▲                                                  │              
     │                                                  │              
     └──────────────────────────────────────────────────┘              
```                                                                        
The PPU cycles through 
OAM Scan (Mode 2) -> Drawing Pixels (Mode 3) -> Horizontal Bank (Mode 0) - Vertical Blank (Mode 1)

|Mode                     |Action                                                 |Duration|Accessible Video Memory|
|-------------------------|-------------------------------------------------------|--------|------------------------|
|OAM Scan (2)             |Search OAM memory for sprites to render                |80 dots|VRAM, CGB palettes|
|Drawing (3)              | Pixels are transferred to LCD                         | 172 - 289 dots | None|
|Horizontal Blank (0)     | Adds time so total cycles can reach 456, PPU paused   | 376 - Mode3 Duration| VRAM, OAM, CGB palettes|
|Vertical Blank (1)       | Wait till next frame                                  |4560 dots|VRAM, OAM, CGB, palettes|

### OAM Mode
- Load sprites to render on the line, runs for 80 ticks
- Sets to drawing Mode once its hits 80 ticks

### Drawing Mode

#### Pipeline
State Machine
```
    +-------+     +--------+     +--------+     +------+     +------+
    |  TILE  | --> | DATA0  | --> | DATA1  | --> | IDLE | --> | PUSH |
    +-------+     +--------+     +--------+     +------+     +------+
        ^                                                        |
        +--------------------------------------------------------+
```

`fetchTile`: 
  - read pixels current x and y value, stored in mapX and mapY
  - stores tileIdx in bgwBuffer[0] if background or window
  - stores sprites in `fetchedEntries` if sprite overlaps with fetchposition

`fetchDataLow`:
  - load low byte of the tile data to bgwBuffer[1]
  - load data for sprites
`fetchDataHigh`:
  - load high byte of the tile data to bgwBuffer[2]
  - load data for sprites
`idle`:
 - wait for timing sync
`push`:
  - pixels are processed, color is extracted, 
  - if sprite is enabled, process sprite color
  - push to pixelFifo which is later read into the videoBuffer which is used by the UI
  

### Hblank Mode
Occurs once a scaneline has finished being rendered.
During this time, CPU can acess VRAM and rest of system can sync up.

### Vblank Mode
Similar to Hblank CPU can acess all memory and rest of systems can synce. This occurs once all scanlines have been written
