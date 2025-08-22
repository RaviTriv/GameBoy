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

  // Bitwise
  void bitAnd();
  void bitOr();
  void bitXor();
  void bitCpl();
  void bitCb();

  // Jump / Call
  void jp();
  void jr();
  void call();

  // Stack
  void pop();
  void push();
  
};