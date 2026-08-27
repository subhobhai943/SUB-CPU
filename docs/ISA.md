# SUB-CPU Instruction Set Architecture (ISA) v1.0

This document defines the complete ISA for the SUB-CPU 16-bit architecture.

---

## Instruction Format

All instructions are 16 bits wide. Instructions that require an immediate value extend to 32 bits (instruction word + 16-bit immediate word).

```
[15:12] - Opcode (4 bits)
[11:9]  - Destination Register (3 bits)
[8:6]   - Source Register A (3 bits)
[5:3]   - Source Register B (3 bits)
[2:0]   - Mode/Flags (3 bits)
```

For immediate instructions, bits [11:0] may encode a short immediate or addressing mode, followed by a 16-bit immediate word.

---

## Register File

| ID  | Name  | Description                  |
|-----|-------|------------------------------|
| 000 | R0    | General purpose              |
| 001 | R1    | General purpose              |
| 010 | R2    | General purpose              |
| 011 | R3    | General purpose              |
| 100 | R4    | General purpose              |
| 101 | R5    | General purpose              |
| 110 | R6    | General purpose / Frame ptr  |
| 111 | R7    | General purpose / Return val |

Special registers (not directly addressable in most instructions):
- **PC** — Program Counter
- **SP** — Stack Pointer (initialized to `0xFFFF`)
- **FLAGS** — Status flags: `Z C N V` (Zero, Carry, Negative, Overflow)

---

## Opcodes

### Arithmetic & Logic

| Mnemonic | Opcode | Description                         | Example            |
|----------|--------|-------------------------------------|--------------------|  
| ADD      | 0x0    | Rd = Ra + Rb                        | `ADD R2, R0, R1`   |
| SUB      | 0x1    | Rd = Ra - Rb                        | `SUB R2, R0, R1`   |
| AND      | 0x2    | Rd = Ra & Rb                        | `AND R2, R0, R1`   |
| OR       | 0x3    | Rd = Ra | Rb                        | `OR  R2, R0, R1`   |
| XOR      | 0x4    | Rd = Ra ^ Rb                        | `XOR R2, R0, R1`   |
| NOT      | 0x5    | Rd = ~Ra                            | `NOT R1, R0`       |
| SHL      | 0x6    | Rd = Ra << Rb                       | `SHL R1, R0, R2`   |
| SHR      | 0x7    | Rd = Ra >> Rb (logical)             | `SHR R1, R0, R2`   |

### Memory

| Mnemonic | Opcode | Description                         | Example            |
|----------|--------|-------------------------------------|--------------------|  
| LDI      | 0x8    | Rd = imm16                          | `LDI R0, 0xFF`     |
| LDR      | 0x9    | Rd = MEM[Ra]                        | `LDR R0, R1`       |
| STR      | 0xA    | MEM[Ra] = Rb                        | `STR R0, R1`       |
| MOV      | 0xB    | Rd = Ra                             | `MOV R1, R0`       |

### Control Flow

| Mnemonic | Opcode | Description                         | Example            |
|----------|--------|-------------------------------------|--------------------|  
| JMP      | 0xC    | PC = imm16                          | `JMP 0x0200`       |
| JZ       | 0xD    | if Z: PC = imm16                    | `JZ  loop`         |
| JNZ      | 0xE    | if !Z: PC = imm16                   | `JNZ loop`         |
| CALL     | 0xF    | Push PC; PC = imm16                 | `CALL my_func`     |

### Stack & Misc (extended via Mode bits)

| Mnemonic | Description                                      |
|----------|--------------------------------------------------|
| PUSH Rd  | SP--; MEM[SP] = Rd                               |
| POP  Rd  | Rd = MEM[SP]; SP++                               |
| RET      | PC = MEM[SP]; SP++                               |
| NOP      | No operation                                     |
| HLT      | Halt execution                                   |

---

## Addressing Modes

| Mode        | Syntax        | Description                              |
|-------------|---------------|------------------------------------------|
| Immediate   | `#42`         | Value encoded directly in instruction    |
| Register    | `R0`          | Value from register                      |
| Direct      | `[0x1000]`    | Value from absolute memory address       |
| Indirect    | `[R1]`        | Value from address stored in register    |

---

## Flags Register

| Bit | Flag | Set When                             |
|-----|------|--------------------------------------|
| 3   | Z    | Result is zero                       |
| 2   | C    | Unsigned carry/borrow occurred       |
| 1   | N    | Result is negative (bit 15 = 1)      |
| 0   | V    | Signed overflow occurred             |

---

## Memory Map

| Range           | Region              |
|-----------------|---------------------|
| `0x0000–0x00FF` | Zero Page (fast access) |
| `0x0100–0x7FFF` | User Program Space  |
| `0x8000–0xEFFF` | Heap / Data         |
| `0xF000–0xFEFF` | MMIO (I/O devices)  |
| `0xFF00–0xFFFF` | Stack               |

---

## Example Programs

### Add Two Numbers

```asm
; hello.sasm — Add R0 + R1 and halt
LDI R0, 10
LDI R1, 32
ADD R2, R0, R1   ; R2 = 42
HLT
```

### Fibonacci (iterative)

```asm
; fib.sasm — Compute Fibonacci(8) in R2
LDI R0, 0        ; a = 0
LDI R1, 1        ; b = 1
LDI R3, 8        ; counter = 8
LDI R4, 0        ; temp

loop:
  ADD R4, R0, R1  ; temp = a + b
  MOV R0, R1      ; a = b
  MOV R1, R4      ; b = temp
  LDI R5, 1
  SUB R3, R3, R5  ; counter--
  JNZ loop        ; if counter != 0, repeat

MOV R2, R0       ; result in R2
HLT
```

---

*SUB-CPU ISA v1.0 — Copyright (C) 2026 Subho. Licensed under GPL v3.*
