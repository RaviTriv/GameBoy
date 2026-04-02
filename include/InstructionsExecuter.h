#pragma once

#include "./Instructions.h"

#include <memory>

class CpuContext;
class InstructionsExecuter
{
public:
  InstructionsExecuter(CpuContext *ctx);
  void execute();

private:
  CpuContext *ctx;

  // Arithmetic
  void add();
  void adc();
  void cp();
  void dec();
  void inc();
  void sub();
  void sbc();

  // Load
  void ld();
  void ldh();

  // Bitwise
  void bitAnd();
  void bitOr();
  void bitXor();
  void bitCpl();

  // CB-prefixed instructions
  void cbRlc();
  void cbRrc();
  void cbRl();
  void cbRr();
  void cbSla();
  void cbSra();
  void cbSwap();
  void cbSrl();
  void cbBit();
  void cbRes();
  void cbSet();

  // Jump / Call
  void jp();
  void jr();
  void call();

  // Stack
  void pop();
  void push();

  // Bit shift
  void rlca();
  void rrca();
  void rla();
  void rra();
  void daa();

  // Bit flag
  void scf();
  void ccf();

  // Interrupt
  void di();
  void ei();

  // Control
  void ret();
  void reti();
  void rst();
  void halt();
};
