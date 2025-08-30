# CPU

## Overview
The CPU reads and executes instructions from memory. It is a state machine that follows a fetch -> decode -> execute cycle. 

- 8 bit, processes 8 bits at a time
- interrupt handling

### State
- Registers: memory that is quick and easy to access
  - `a` 8 bits, accumulator, holds values from arithmetic operations
  - `f` 8 bits, flags
  - `b` 8 bit general purpose
  - `c` 8 bit general purpose
  - `e` 8 bit general purpose
  - `h` 8 bit general purpose
  - `l` 8 bit general purpose
  - `pc` 16 bit, program counter, keeps track of instruction to execute
  - `sp` 16 bit, stack pointer, used to keep track of stack, which is used for storing variables and addresses

### Instruction Set Architecture (ISA)

- Arimethic
- Load
- Bitwise
- Jump
- Stack
- Interrupt
- Control

