# SUB64 — A 64-bit CPU Architecture

> An original **64-bit CPU architecture** designed from scratch under the **SUB** brand.  
> SUB64 is **not x86, not ARM, not RISC-V** — it is its own thing.

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Architecture](https://img.shields.io/badge/Architecture-64--bit-blueviolet.svg)]()
[![ISA Style](https://img.shields.io/badge/ISA-RISC%2BVLIW%20Hybrid-orange.svg)]()
[![Status](https://img.shields.io/badge/Status-WIP-yellow.svg)]()

---

## What Makes SUB64 Different?

Most hobby CPUs are simplified RISC-V or MIPS clones. SUB64 is built with original ideas:

| Feature | x86-64 | ARM64 | RISC-V | **SUB64** |
|---|---|---|---|---|
| Word Size | 64-bit | 64-bit | 64-bit | **64-bit** |
| Instruction Width | Variable (1–15 B) | Fixed 32-bit | Fixed 32-bit | **Fixed 32-bit + 64-bit WIDE mode** |
| Register Count | 16 | 31 | 32 | **24 GPR + 8 Vector** |
| Calling Convention | Complex | AAPCS64 | RISC-V CC | **SUB ABI (custom)** |
| ISA Style | CISC | RISC | RISC | **RISC + VLIW dual-issue slots** |
| Predication | Partial | Full | None | **Full per-instruction predication** |
| Immediate Width | Up to 32-bit | 16-bit | 12-bit | **Up to 48-bit inline immediate** |
| Memory Model | TSO | Weak | Weak | **SUB-MO (custom ordered)** |
| Privilege Levels | Ring 0–3 | EL0–EL3 | M/S/U | **SUB Ring 0–2 (Kernel/User/Guest)** |

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│                     SUB64 Core                          │
│  ┌─────────┐  ┌─────────┐  ┌────────────┐  ┌────────┐  │
│  │  Fetch  │→ │ Decode  │→ │  Execute   │→ │  WB    │  │
│  │  Unit   │  │ (Dual   │  │ (ALU/FPU/  │  │ Stage  │  │
│  │         │  │  Slot)  │  │  VPU/MEM)  │  │        │  │
│  └─────────┘  └─────────┘  └────────────┘  └────────┘  │
│                                                         │
│  Registers: X0–X23 (GPR 64-bit) + V0–V7 (256-bit Vec)  │
│  Special:   PC, SP, LR, FLAGS, KVEC, UVEC              │
│  Memory:    48-bit virtual address space (256 TB)       │
└─────────────────────────────────────────────────────────┘
```

---

## Repository Structure

```
SUB-CPU/
├── docs/
│   ├── ISA.md            # Full ISA reference
│   ├── ABI.md            # Calling convention & SUB ABI
│   ├── MEMORY_MODEL.md   # SUB-MO memory ordering
│   └── PRIV.md           # Privilege levels & rings
├── emulator/
│   ├── cpu.c             # Core emulator (fetch/decode/execute)
│   ├── cpu.h
│   ├── fpu.c             # Floating-point unit
│   ├── vpu.c             # Vector processing unit
│   └── main.c
├── assembler/
│   ├── lexer.c
│   ├── parser.c
│   ├── encoder.c
│   └── main.c
├── include/
│   └── sub64.h           # Master header
├── examples/
│   ├── hello.sasm
│   ├── fib.sasm
│   └── vector_add.sasm
├── Makefile
├── LICENSE
└── README.md
```

---

## Registers

### General Purpose Registers (64-bit)

| Reg | ABI Name | Role |
|-----|----------|------|
| X0 | zero | Always 0 (writes ignored) |
| X1 | ra | Return address |
| X2 | sp | Stack pointer |
| X3 | fp | Frame pointer |
| X4–X7 | a0–a3 | Function arguments / return values |
| X8–X15 | t0–t7 | Caller-saved temporaries |
| X16–X23 | s0–s7 | Callee-saved saved registers |

### Vector Registers (256-bit)

| Reg | Width | Use |
|-----|-------|-----|
| V0–V7 | 256-bit | SIMD: 4×f64, 8×f32, 4×i64, 8×i32, 16×i16, 32×i8 |

### Special Registers

| Reg | Purpose |
|-----|---------|
| PC | Program counter (48-bit effective) |
| FLAGS | ZCNVIP (Zero/Carry/Neg/oVerflow/Interrupt/Parity) |
| LR | Link register (saved PC for CALL) |
| KVEC | Kernel interrupt vector base |
| UVEC | User trap vector base |
| RING | Current privilege ring (0=kernel, 1=user, 2=guest) |

---

## Quick ISA Glimpse

```asm
; SUB64 Assembly — hello.sasm

  LDIM  X4, 100      ; X4 = 100  (48-bit immediate)
  LDIM  X5, 200      ; X5 = 200
  ADD   X6, X4, X5  ; X6 = 300

; Predicated move: only if Zero flag set
  MOVZ  X7, X6      ; X7 = X6 if Z==1

; Dual-issue WIDE instruction (two ops in one 64-bit word)
  WIDE  ADD X8, X4, X5 | SUB X9, X5, X4

  HLT
```

See [`docs/ISA.md`](docs/ISA.md) for the full reference.

---

## Building & Running

```bash
git clone https://github.com/subhobhai943/SUB-CPU.git
cd SUB-CPU
make                # Build both emulator (sub64-emu) and assembler (sasm)

# Assemble an example program:
./sasm examples/hello.sasm -o hello.bin

# Run on the SUB64 emulator (-d dumps CPU register state):
./sub64-emu hello.bin -d

# Disassemble a binary:
./sdisasm hello.bin

# Run the comprehensive test suite:
make test
```

Binaries are also placed in `bin/` (`bin/emulator`, `bin/assembler`, `bin/disassembler`).

---

## Roadmap

- [x] ISA design v1.0 (64-bit, RISC+VLIW hybrid)
- [x] Register file design (24 GPR + 8 VEC)
- [x] Memory model specification (SUB-MO)
- [x] Privilege ring design (Ring 0/1/2)
- [x] Emulator core in C (Fetch / Decode / Execute, 70+ opcodes, 4 GB RAM)
- [x] Assembler (SASM → 32-bit & 64-bit binary, label resolution)
- [x] Dual-issue WIDE execution engine
- [x] VPU / SIMD simulation (256-bit vectors, 4×64-bit lanes)
- [x] Kernel ABI & syscall table (sys_exit, sys_write, sys_read)
- [x] FPU 64-bit IEEE-754 simulation (FADD, FSUB, FMUL, FDIV, FSQRT, FCMP, FCVT)
- [x] Interrupt / exception IVT model (Ring 0/1/2, KVEC base, traps, SYSRET)
- [x] Disassembler tool (binary → readable SASM assembly with WIDE support)
- [ ] FPGA RTL port (Verilog)

---

<p align="center">Built with ❤️ by <a href="https://github.com/subhobhai943">Subho</a> · SUB Brand · 2026 · GPL v3</p>
