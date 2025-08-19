#pragma once

#include "./Instructions.h"

#include <memory>

class CPU;
class InstructionsExecuter
{
public:
  InstructionsExecuter(CPU *cpu);
  void execute();

private:
  CPU *cpu;
};