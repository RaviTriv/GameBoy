# CPU

## Overview
The CPU reads and executes instructions from memory. It is a state machine that follows a fetch -> decode -> execute cycle. 

- 8 bit, processes 8 bits at a time
- interrupt handling

### State
- Registers: memory that is quick and easy to access
  - `a` 8 bits, accumulator, holds values from arithmetic operations
  - `f` 8 bits, flags
    - Bit 7: Z (Zero flag), if some operations result was zero or values match from `cp` operation
    - Bit 6: N (Subtract flag), is subtraction occured in last operation set
    - Bit 5: H (Half-carry flag), is set if carry operation occured in lower 4 bits
    - Bit 4: C (Carry flag), is set if carry operation occured or register a is smaller value when executing `cp` 
    - Bits 3-0: Unused
  - `b` 8 bit general purpose
  - `c` 8 bit general purpose
  - `d` 8 bit general purpose
  - `e` 8 bit general purpose
  - `h` 8 bit general purpose
  - `l` 8 bit general purpose
  - `pc` 16 bit, program counter, keeps track of instruction to execute
  - `sp` 16 bit, stack pointer, used to keep track of stack, which is used for storing variables, return addresses, function arguements, 

- TODO: List how registers can be used together
### Instruction Set Architecture (ISA)

- Arimethic 
- Load
- Bitwise
- Jump
- Stack
- Interrupt
- Control

TODO: List key operations of each type