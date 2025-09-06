# PPU
The PPU is responsible for reading and processing data from memory (vram) to render Gameboy graphics. 

## Registers
 - `lcdc`: LCD control
 - `lcds`: LCD status
 - `scy`: Scroll y
 - `scx`: Scroll x
 - `ly`: Current scanline
 - `lyc`: Scanline comparison
 - `wy`: Window y
 - `wx`: Window x
 - `bgp`: Background palette data
 - `obp0`: Object 0 palette data
 - `obp1`: Object 1 palette data
 - `dma`: Direct memory transfer

## Display
The gameboy graphics are made up of tiles and several layers.
### Tiles
- 8*8 pixel block
- Every display component is built with tiles
### Background
- 256 * 256 pixel background layer
- scrollable
### Window
- 256 * 256 pixels
- Overlays over the background.
- Position set through window registers
- Partial and fully visibility is toggable
- Can be used for things such as text dialog boxes in pokemon
### Viewport
- 160 * 144 pixels, the full screen that is actually visible
### Sprite
- 8 * 8 or 8 * 16 pixels
- Actual objects, characters and other interactive elements
- Appears in front or behind of background based on set priority number 
- 40 Total, with max 10 per line

## Rendering
The Game Boy renders the display one scanline at a time, from left to right and top to bottom.

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

|Mode                     |Action                                                 |Duration|Accessible Video Memory|
|-------------------------|-------------------------------------------------------|--------|------------------------|
|OAM Scan (2)             |Search OAM memory for sprites to render                |80 dots|VRAM|
|Drawing (3)              | Pixels are transferred to LCD                         | 172 - 289 dots | None|
|Horizontal Blank (0)     | Adds time so total cycles can reach 456, PPU paused   | 376 - Mode3 Duration| VRAM, OAM, CGB palettes|
|Vertical Blank (1)       | Wait till next frame                                  |4560 dots|VRAM, OAM, CGB, palettes|

### OAM Mode
OAM Memory is searched to to find sprites that should be loaded.

### Drawing Mode
Pixels are processed and transfered to the LCD throught the Pixel Pipeline.

#### Pipeline
The pixel pipeline is another state machine.
```
  ┌───────┐       ┌────────┐       ┌────────┐       ┌──────┐       ┌──────┐
  │       │       │        │       │        │       │      │       │      │
  │ TILE  ├──────►│ DATA0  ├──────►│ DATA1  ├──────►│ IDLE ├──────►│ PUSH │
  │       │       │        │       │        │       │      │       │      │
  └───────┘       └────────┘       └────────┘       └──────┘       └──────┘
      ▲                                                                │
      │                                                                │
      └────────────────────────────────────────────────────────────────┘
```
| State | Duration | Description |
|-------|----------|-------------|
| **Fetch Tile** | 2 dots | Reads tile index based on current coordinates (mapX, mapY). For background/window, stores index in bgwBuffer[0]. For sprites, stores overlapping sprites in fetchedEntries. |
| **Fetch Data 0** | 2 dots | Reads the low byte of tile data into bgwBuffer[1]. Also loads corresponding sprite data if needed. |
| **Fetch Data 1** | 2 dots | Reads the high byte of tile data into bgwBuffer[2]. Also loads corresponding sprite data if needed. |
| **Idle** | 2 dots | Waits for timing synchronization. Ensures consistent timing between fetches. |
| **Push** | Variable | Processes pixels by extracting colors from the tile data. Combines background/window with sprite data according to priority rules. Pushes 8 pixels to the FIFO queue which feeds the display buffer. |
  
### Hblank Mode
Occurs once a scaneline has finished being rendered the PPU is temporarily paused. At this time the CPU has free acess to VRAM, OAM, and palette data.

### Vblank Mode
Once all 144 scalines have been rendered vblank occurs. Similar to Hblank the PPU is temporarily paused and the CPU has acess to VRAM, OAM, and palette data.
