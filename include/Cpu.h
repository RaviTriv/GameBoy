#pragma once

#include "./InstructionsDecoder.h"

#include <cstdint>
#include <memory>
#include <functional>

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

class Bus;
class CPU
{
public:
  struct State
  {
    struct Registers registers;
    uint8_t currentOpcode;
  };

  using CycleCallback = std::function<void(int)>;

  CPU(CycleCallback cycleCallback, std::shared_ptr<Bus> bus);

  void step();

private:
  CycleCallback cycleCallback;
  InstructionsDecoder decoder;
  std::shared_ptr<Bus> bus;

  State state;
  void fetch();
  void decode();
  void execute();
};