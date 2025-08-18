#include "../../../include/Cpu.h"
#include "../../../include/Logger.h"

void CPU::step()
{
  Logger::GetLogger()->info("TEST: {}", static_cast<int>(decoder.getInstruction(0x04).reg1));
}