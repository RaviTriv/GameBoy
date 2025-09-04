#include "../include/Gameboy.h"
#include <string>
#include <vector>
#include <algorithm>

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    throw std::invalid_argument("Usage: gameboy <rom_path> [--trace] [--loadSave]");
  }

  std::string romPath;
  bool trace = false;
  bool loadSave = false;

  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];

    if (arg == "--trace")
    {
      trace = true;
    }
    else if (arg == "--loadSave")
    {
      loadSave = true;
    }
    else
    {
      if (arg.substr(0, 2) != "--")
      {
        romPath = arg;
      }
    }
  }

  if (romPath.empty())
  {
    throw std::invalid_argument("No ROM file provided.");
  }

  GameBoy gameboy;

  gameboy.init(romPath, trace, loadSave);
  gameboy.run();
  return 0;
}