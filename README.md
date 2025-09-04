# GameBoy
DMG-01 Game Boy Emulator

Play games, tested with few games such as, pokemon red, tetris, zelda's awakening

## Building
```console
mkdir build && cd build
cmake ..
make
```

## Running
```console
./gameboy ${PATH_TO_ROM} [--trace] [--loadSave]
```

| Argument | Required | Description |
|----------|----------|-------------|
| `<rom_path>` | Yes | Path to the GameBoy ROM file (.gb or .gbc) |
| `--trace` | No | Enables debug trace mode |
| `--loadSave` | No | Loads saved game data if available |
| `--fastForward` | No | Speeds up the game when enabled |

## Documentation
Core components are documented under [`docs`](docs/).

## Controls

| Keyboard Key | GameBoy Button | Function |
|--------------|----------------|----------|
| Z            | B              | B button |
| X            | A              | A button |
| Enter/Return | Start          | Start button |
| Tab          | Select         | Select button |
| ↑ (Up)       | D-pad Up       | Move up |
| ↓ (Down)     | D-pad Down     | Move down |
| ← (Left)     | D-pad Left     | Move left |
| → (Right)    | D-pad Right    | Move right |
| S            | N/A            | Save state |
| F            | N/A            | Toggle fast forward |

## Screenshots
![Tetris](images/tetris.png)
![Zelda Link's Awakening](images/zelda.png)
![Pokemon Red](images/pokemonRed.png)