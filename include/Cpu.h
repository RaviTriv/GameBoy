#pragma once

#include <cstdint>
#include <memory>

struct Registers
{
  uint8_t a;
  uint8_t f;
  uint8_t b;
  uint8_t c;
  uint8_t d;
  uint8_t e;
  uint8_t h;
  uint8_t l;
  uint16_t pc;
  uint16_t sp;
};

struct State
{
  struct Registers registers;
};

class CPU
{
public:
  void step();

private:
  State state;
  void fetch();
  void decode();
  void execute();
};