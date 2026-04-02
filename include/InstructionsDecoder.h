#pragma once

#include "./OpcodeTable.h"

#include <memory>

class CpuContext;
class InstructionsDecoder
{
public:
  InstructionsDecoder(CpuContext *ctx);

  void decode(uint8_t opcode);

private:
  CpuContext *ctx;
  using AddressModeHandler = void (InstructionsDecoder::*)();
  static AddressModeHandler addressModeHandlers[];

  [[nodiscard]] static const Instruction &getInstruction(uint8_t opcode);
  [[nodiscard]] static const Instruction &getCBInstruction(uint8_t opcode);
  void imp();
  void r();
  void r_r();
  void r_d8();
  void r_d16();
  void mr_r();
  void r_mr();
  void r_hli();
  void r_hld();
  void hli_r();
  void hld_r();
  void r_a8();
  void a8_r();
  void d8();
  void hl_spr();
  void a16_r();
  void mr_d8();
  void mr();
  void r_a16();
  void none();
};
