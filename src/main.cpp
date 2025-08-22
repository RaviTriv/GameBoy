#include "../include/Gameboy.h"

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    throw std::invalid_argument("No ROM file provided.");
  }

  GameBoy gameboy;
  gameboy.init(std::string(argv[1]));
  gameboy.run();
  return 0;
}