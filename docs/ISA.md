# SUB64 Instruction Set Architecture — v1.0

SUB64 is a **64-bit RISC/VLIW hybrid** architecture with full per-instruction predication,
48-bit inline immediates, and dual-issue WIDE mode. It is not derived from any existing ISA.

---

## 1. Instruction Formats

SUB64 has **two instruction widths**:

### Standard (32-bit)
```
[31:26] OPCODE  (6 bits)   — 64 possible base opcodes
[25:24] PRED    (2 bits)   — predicate condition
[23:19] RD      (5 bits)   — destination register
[18:14] RA      (5 bits)   — source A
[13:9]  RB      (5 bits)   — source B
[8:6]   FUNC    (3 bits)   — sub-function / shift amount class
[5:0]   IMM6    (6 bits)   — small immediate or mode flags
```

### WIDE (64-bit) — dual-issue
```
[63]    WIDE=1             — marks this as a 64-bit instruction
[62:32] SLOT_A (31 bits)  — first operation (compact encoding)
[31]    WIDE=1             — always 1
[30:0]  SLOT_B (31 bits)  — second operation issued in parallel
```
Both slots are executed in the **same cycle** on independent execution units.

### IMML (32-bit + 48-bit immediate extension)
```
Instruction word (32-bit) with OPCODE=LDIM, followed by
a 48-bit immediate in the next 6 bytes (little-endian).
Total size: 80 bits (10 bytes), aligned to 16-byte boundary.
```

---

## 2. Predication

Every standard instruction has a 2-bit **PRED** field:

| PRED | Meaning | Condition |
|------|---------|----------|
| 00 | AL | Always execute (default) |
| 01 | EQ | Execute only if Z=1 |
| 10 | NE | Execute only if Z=0 |
| 11 | CS | Execute only if C=1 (carry set) |

This eliminates most conditional branches and enables if-conversion without branch mispredictions.

---

## 3. Opcode Table

### Arithmetic (OPCODE 0x00–0x0F)

| Mnemonic | OPCODE | Operation | Example |
|----------|--------|-----------|--------|
| ADD | 0x00 | Rd = Ra + Rb | `ADD X2, X0, X1` |
| ADDI | 0x01 | Rd = Ra + imm6 | `ADDI X2, X0, 10` |
| SUB | 0x02 | Rd = Ra - Rb | `SUB X2, X0, X1` |
| SUBI | 0x03 | Rd = Ra - imm6 | `SUBI X2, X0, 1` |
| MUL | 0x04 | Rd = Ra × Rb (low 64) | `MUL X3, X1, X2` |
| MULH | 0x05 | Rd = (Ra × Rb) >> 64 (high) | `MULH X4, X1, X2` |
| DIV | 0x06 | Rd = Ra ÷ Rb (signed) | `DIV X3, X1, X2` |
| REM | 0x07 | Rd = Ra mod Rb | `REM X3, X1, X2` |
| NEG | 0x08 | Rd = -Ra | `NEG X1, X0` |
| ABS | 0x09 | Rd = |Ra| | `ABS X1, X0` |

### Logic & Bit (OPCODE 0x10–0x1F)

| Mnemonic | OPCODE | Operation | Example |
|----------|--------|-----------|--------|
| AND | 0x10 | Rd = Ra & Rb | `AND X2, X0, X1` |
| ANDI | 0x11 | Rd = Ra & imm6 | `ANDI X2, X0, 0xF` |
| OR | 0x12 | Rd = Ra \| Rb | `OR X2, X0, X1` |
| ORI | 0x13 | Rd = Ra \| imm6 | `ORI X2, X0, 1` |
| XOR | 0x14 | Rd = Ra ^ Rb | `XOR X2, X0, X1` |
| NOT | 0x15 | Rd = ~Ra | `NOT X1, X0` |
| SHL | 0x16 | Rd = Ra << Rb | `SHL X2, X0, X1` |
| SHR | 0x17 | Rd = Ra >> Rb (logical) | `SHR X2, X0, X1` |
| SAR | 0x18 | Rd = Ra >> Rb (arithmetic) | `SAR X2, X0, X1` |
| ROL | 0x19 | Rd = Ra rotl Rb | `ROL X2, X0, X1` |
| ROR | 0x1A | Rd = Ra rotr Rb | `ROR X2, X0, X1` |
| CLZ | 0x1B | Rd = count leading zeros Ra | `CLZ X1, X0` |
| CTZ | 0x1C | Rd = count trailing zeros Ra | `CTZ X1, X0` |
| POPC | 0x1D | Rd = popcount(Ra) | `POPC X1, X0` |
| BEXT | 0x1E | Rd = extract bit Ra[Rb] | `BEXT X1, X0, X2` |
| BDEP | 0x1F | Rd = deposit bit | `BDEP X1, X0, X2` |

### Memory (OPCODE 0x20–0x2F)

| Mnemonic | OPCODE | Operation | Example |
|----------|--------|-----------|--------|
| LD64 | 0x20 | Rd = MEM64[Ra + imm6] | `LD64 X0, X2, 8` |
| LD32 | 0x21 | Rd = MEM32[Ra + imm6] (zero-ext) | `LD32 X0, X2, 0` |
| LD16 | 0x22 | Rd = MEM16[Ra + imm6] | `LD16 X0, X2, 0` |
| LD8 | 0x23 | Rd = MEM8[Ra + imm6] | `LD8 X0, X2, 0` |
| ST64 | 0x24 | MEM64[Ra + imm6] = Rb | `ST64 X2, X1, 0` |
| ST32 | 0x25 | MEM32[Ra + imm6] = Rb | `ST32 X2, X1, 0` |
| ST16 | 0x26 | MEM16[Ra + imm6] = Rb | `ST16 X2, X1, 0` |
| ST8 | 0x27 | MEM8[Ra + imm6] = Rb | `ST8 X2, X1, 0` |
| LDIM | 0x28 | Rd = imm48 (next 6 bytes) | `LDIM X0, 0xDEADBEEFCAFE` |
| LEA | 0x29 | Rd = Ra + imm6 (no mem access) | `LEA X0, X2, 16` |
| PUSH | 0x2A | SP-=8; MEM[SP]=Ra | `PUSH X1` |
| POP | 0x2B | Ra=MEM[SP]; SP+=8 | `POP X1` |
| XCHG | 0x2C | atomic swap MEM[Ra] ↔ Rb | `XCHG X0, X1` |
| CAS | 0x2D | compare-and-swap | `CAS X0, X1, X2` |

### Control Flow (OPCODE 0x30–0x3F)

| Mnemonic | OPCODE | Operation | Example |
|----------|--------|-----------|--------|
| JMP | 0x30 | PC = Ra | `JMP X1` |
| JMPI | 0x31 | PC = PC + imm16 (rel) | `JMPI loop` |
| CALL | 0x32 | LR = PC+4; PC = Ra | `CALL X1` |
| CALLI | 0x33 | LR = PC+4; PC = PC + imm16 | `CALLI my_func` |
| RET | 0x34 | PC = LR | `RET` |
| BEQ | 0x35 | if Ra==Rb: PC+=imm6 | `BEQ X0, X1, done` |
| BNE | 0x36 | if Ra!=Rb: PC+=imm6 | `BNE X0, X1, loop` |
| BLT | 0x37 | if Ra<Rb (signed): branch | `BLT X0, X1, err` |
| BGE | 0x38 | if Ra>=Rb (signed): branch | `BGE X0, X1, ok` |
| BLTU | 0x39 | unsigned less-than branch | `BLTU X0, X1, err` |
| BGEU | 0x3A | unsigned >=  branch | `BGEU X0, X1, ok` |
| LOOP | 0x3B | X23--; if X23!=0: branch | `LOOP top` (hardware loop) |
| SYSCALL | 0x3C | trap to kernel (Ring 0) | `SYSCALL` |
| SYSRET | 0x3D | return from kernel trap | `SYSRET` |
| FENCE | 0x3E | memory barrier | `FENCE` |
| HLT | 0x3F | halt CPU | `HLT` |

### Comparison (OPCODE 0x40–0x4F)

| Mnemonic | OPCODE | Operation |
|----------|--------|-----------|
| CMP | 0x40 | set FLAGS from Ra - Rb (discard result) |
| CMPI | 0x41 | set FLAGS from Ra - imm6 |
| TST | 0x42 | set FLAGS from Ra & Rb |
| TSTI | 0x43 | set FLAGS from Ra & imm6 |
| SEQ | 0x44 | Rd = (Ra == Rb) ? 1 : 0 |
| SNE | 0x45 | Rd = (Ra != Rb) ? 1 : 0 |
| SLT | 0x46 | Rd = (Ra < Rb signed) ? 1 : 0 |
| SLTU | 0x47 | Rd = (Ra < Rb unsigned) ? 1 : 0 |

### Move & Select (OPCODE 0x50–0x5F)

| Mnemonic | OPCODE | Operation |
|----------|--------|-----------|
| MOV | 0x50 | Rd = Ra |
| MOVZ | 0x51 | Rd = Ra if Z==1 |
| MOVNZ | 0x52 | Rd = Ra if Z==0 |
| MOVC | 0x53 | Rd = Ra if C==1 |
| MOVN | 0x54 | Rd = Ra if N==1 |
| SEL | 0x55 | Rd = (FLAGS.Z) ? Ra : Rb |
| ZEXT32 | 0x56 | Rd = Ra & 0xFFFFFFFF |
| SEXT32 | 0x57 | Rd = sign_extend(Ra[31:0]) |
| SEXT16 | 0x58 | Rd = sign_extend(Ra[15:0]) |
| SEXT8 | 0x59 | Rd = sign_extend(Ra[7:0]) |

### Vector / SIMD (OPCODE 0x20–0x2F on VPU lane)

| Mnemonic | Operation |
|----------|-----------|
| VADD.64 | V_d = V_a + V_b (4×f64 lanes) |
| VSUB.64 | V_d = V_a - V_b |
| VMUL.64 | V_d = V_a * V_b |
| VDOT.64 | Scalar dot product of V_a · V_b → Rd |
| VLOAD | V_d = MEM[Ra] (256-bit aligned) |
| VSTORE | MEM[Ra] = V_s |
| VSHUFFLE | V_d = shuffle(V_a, imm8 mask) |
| VBROADCAST | V_d = {Ra, Ra, Ra, Ra} |

---

## 4. FLAGS Register

```
Bit 63–6: Reserved
Bit 5: IP — Interrupt Pending
Bit 4: P  — Parity
Bit 3: Z  — Zero
Bit 2: C  — Carry
Bit 1: N  — Negative
Bit 0: V  — Overflow
```

---

## 5. Memory Map (48-bit Virtual)

| Range | Region |
|-------|--------|
| `0x0000_0000_0000` – `0x0000_7FFF_FFFF` | User space (2 GB default) |
| `0x0000_8000_0000` – `0x00FF_FFFF_EFFF` | Extended user / heap |
| `0xFF00_0000_0000` – `0xFFFF_EFFF_FFFF` | Kernel space |
| `0xFFFF_F000_0000` – `0xFFFF_FEFF_FFFF` | MMIO (memory-mapped I/O) |
| `0xFFFF_FF00_0000` – `0xFFFF_FFFF_FFFF` | Kernel stack + IVT |

Physical address width is implementation-defined (minimum 40-bit).

---

## 6. WIDE Dual-Issue Example

```asm
; Execute ADD and SUB in the same cycle:
WIDE  ADD X6, X4, X5 | SUB X7, X5, X4

; Execute two loads in parallel:
WIDE  LD64 X8, X2, 0 | LD64 X9, X3, 0
```

Constraints: SLOT_A and SLOT_B must target **different registers** and must not be memory ops to the same address.

---

## 7. Privilege Rings

| Ring | Name | Can Access |
|------|------|------------|
| 0 | Kernel | All instructions, all registers, all memory |
| 1 | User | Subset of ISA, no KVEC/RING writes, no MMIO |
| 2 | Guest | Hypervisor-controlled sandbox |

Ring transitions happen via `SYSCALL` (1→0) and `SYSRET` (0→1), or via interrupt vectors.

---

*SUB64 ISA v1.0 — Copyright (C) 2026 Subho. GPL v3.*
