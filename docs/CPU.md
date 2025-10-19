# CPU
The Central Processing Unit (CPU) reads and executes instructions from memory. It processes 8-bit chunks of data in a simple fetch-decode-execute cycle.

```
┌───────────┐      ┌──────────┐      ┌───────────┐
│           │      │          │      │           │
│   FETCH   ├─────►│  DECODE  ├─────►│  EXECUTE  │
│           │      │          │      │           │
└─────┬─────┘      └──────────┘      └─────┬─────┘
      ▲                                    │
      │                                    │
      └────────────────────────────────────┘
```

| State | Description |
|-------|-------------|
| FETCH | Read instruction that PC points to |
| DECODE | Use opcode returned from fetch to find operation |
| EXECUTE | Perform operation, update registers and flags |

## Registers

### 8-bit Registers
- **`a`**: Accumulator, used for arithmetic and logic operations
- **`f`**: flags
  - Bit 7: **Z** (Zero flag) - Set when result is zero or values match in compare operations
  - Bit 6: **N** (Subtract flag) - Set if last operation was a subtraction
  - Bit 5: **H** (Half-carry flag) - Set if carry occurred from bit 3 to bit 4
  - Bit 4: **C** (Carry flag) - Set if carry occurred from bit 7, or for borrow in subtraction
  - Bits 3-0: Unused
- **`b`**, **`c`**, **`d`**, **`e`**, **`h`**, **`l`**: General-purpose registers

### 16-bit Registers
- **`pc`**: Program counter, points to the next instruction to execute
- **`sp`**: Stack pointer, points to the top of the stack (used for function calls, interrupts, and temporary storage)

### Register Pairing
The 8-bit registers can be paired to form 16-bit registers for 16-bit operations:
- **`af`**: Accumulator and flags
- **`bc`**: General purpose
- **`de`**: General purpose
- **`hl`**: General purpose

## Instruction Set Architecture (ISA)
The CPU can perform the following operations

### Arithmetic Operations
- **`ADD`**, **`ADC`** (Add with carry): Add values to accumulator
- **`SUB`**, **`SBC`** (Subtract with carry): Subtract values from accumulator
- **`INC`**, **`DEC`**: Increment/decrement registers or memory
- **`CP`**: Compare (set flags)
- **`DAA`**: Decimal adjust accumulator
- **`CPL`**: Complement accumulator

### Logical Operations
- **`AND`**: Logical AND with accumulator
- **`OR`**: Logical OR with accumulator
- **`XOR`**: Logical XOR with accumulator
- **`BIT`**: Test bit in register
- **`SET`**, **`RES`**: Set or reset specific bit

### Rotate and Shift
- **`RLC`**, **`RRC`**: Rotate left/right through carry
- **`RL`**, **`RR`**: Rotate left/right
- **`SLA`**, **`SRA`**, **`SRL`**: Shift left arithmetic, shift right arithmetic/logical
- **`SWAP`**: Swap upper and lower nibbles

### Load Operations
- **`LD`**: Load 8-bit or 16-bit values between registers, memory, or immediate values
- **`LDH`**: Load to/from high memory (`0xFF00` + offset)
- **`LDI`**, **`LDD`**: Load with increment/decrement of `hl`

### Jump and Call Operations
- **`JP`**: Jump to address
- **`JR`**: Jump relative 
- **`CALL`**: Call subroutine
- **`RET`**, **`RETI`**: Return from subroutine / interrupt
- **`RST`**: Restart

### Stack Operations
- **`PUSH`**: Push 16-bit register pair onto stack
- **`POP`**: Pop 16-bit value from stack into register pair

### Interrupt Control
- **`EI`**: Enable interrupts
- **`DI`**: Disable interrupts 
- **`RETI`**: Return from interrupt

### Control Flow
- **`HALT`**: Halt CPU until interrupt occurs
- **`STOP`**: Stop CPU and LCD
- **`NOP`**: No operation