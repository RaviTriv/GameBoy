#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "Gameboy.h"

int main(int argc, char **argv) {
  try {
    if (argc < 2) {
      throw std::invalid_argument(
          "Usage: gameboy <rom_path> [--trace] [--loadSave]");
    }

    std::string romPath;
    bool trace = false;
    bool loadSave = false;
    bool fastForward = false;

    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];

      if (arg == "--trace") {
        trace = true;
      } else if (arg == "--loadSave") {
        loadSave = true;
      } else if (arg == "--fastForward") {
        fastForward = true;
      } else {
        if (!arg.starts_with("--")) {
          romPath = arg;
        }
      }
    }

    if (romPath.empty()) {
      throw std::invalid_argument("No ROM file provided.");
    }

    GameBoy gameboy;
    gameboy.init(romPath, trace, loadSave, fastForward);
    gameboy.run();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Failed to load ROM: " << e.what() << '\n';
    return 1;
  }
}