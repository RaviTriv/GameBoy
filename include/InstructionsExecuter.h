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

  // Arithmetic
  void add();
  void adc();
  void cp();
  void dec();
  void inc();
  void sub();
  void sbc();
};