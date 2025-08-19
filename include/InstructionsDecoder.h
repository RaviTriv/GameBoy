#include "./Instructions.h"

#include <functional>
#include <memory>

struct DecodedInstructionState
{
};

class InstructionsDecoder
{
public:
  void decode(uint8_t opcode);
  using CycleCallback = std::function<void(int)>;

  InstructionsDecoder(CycleCallback cycleCallback);

private:
  CycleCallback cycleCallback;

  static const Instruction &getInstruction(uint8_t opcode);
};