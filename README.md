# SUB-CPU Architecture

> A custom 16-bit CPU architecture designed and built from scratch under the **SUB** brand.

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Architecture](https://img.shields.io/badge/Architecture-16--bit-green.svg)]()
[![Status](https://img.shields.io/badge/Status-WIP-orange.svg)]()

---

## What is SUB-CPU?

SUB-CPU is a fully custom 16-bit CPU architecture created from scratch. It includes:

- A clean, minimal **Instruction Set Architecture (ISA)**
- A software **emulator** written in C
- A hand-written **assembler** (`.sasm` → binary)
- A growing **toolchain** for programs targeting SUB-CPU
- Clear **documentation** for every component

This project is part of the SUB brand — a personal systems-level engineering effort by [Subho](https://github.com/subhobhai943).

---

## Architecture Overview

| Feature | Value |
|---|---|
| Word Size | 16-bit |
| Registers | 8 general-purpose (R0–R7) + PC + SP + FLAGS |
| Addressing Modes | Immediate, Register, Direct, Indirect |
| Memory | 64 KB (16-bit address space) |
| Endianness | Little-endian |
| Instruction Width | Fixed 16-bit (some 32-bit for immediates) |

---

## Repository Structure

```
SUB-CPU/
├── docs/               # Architecture documentation
│   └── ISA.md          # Full Instruction Set Reference
├── emulator/           # C-based emulator source
│   ├── cpu.c
│   ├── cpu.h
│   └── main.c
├── assembler/          # SASM assembler source
│   ├── lexer.c
│   ├── parser.c
│   └── main.c
├── examples/           # Example .sasm programs
│   ├── hello.sasm
│   └── fib.sasm
├── include/            # Shared headers
│   └── sub_cpu.h
├── Makefile
├── LICENSE
└── README.md
```

---

## Getting Started

### Build the Emulator

```bash
git clone https://github.com/subhobhai943/SUB-CPU.git
cd SUB-CPU
make emulator
```

### Build the Assembler

```bash
make assembler
```

### Run a Program

```bash
./assembler examples/hello.sasm -o hello.bin
./emulator hello.bin
```

---

## Registers

| Register | Purpose |
|---|---|
| R0 – R7 | General-purpose 16-bit registers |
| PC | Program Counter |
| SP | Stack Pointer (starts at 0xFFFF) |
| FLAGS | Zero (Z), Carry (C), Negative (N), Overflow (V) |

---

## Quick ISA Glimpse

```asm
; Load immediate value 42 into R0
LDI R0, 42

; Add R0 + R1 → R2
ADD R2, R0, R1

; Store R2 into memory address 0x1000
STR R2, 0x1000

; Halt
HLT
```

See [`docs/ISA.md`](docs/ISA.md) for the full instruction reference.

---

## Roadmap

- [x] ISA design v1.0
- [ ] Emulator (C)
- [ ] Assembler (SASM)
- [ ] Standard library
- [ ] Interrupt & I/O model
- [ ] Pipeline simulation
- [ ] FPGA port (stretch goal)

---

## License

This project is licensed under the **GNU General Public License v3.0**.
See [LICENSE](LICENSE) for details.

---

<p align="center">Built with ❤️ by <a href="https://github.com/subhobhai943">Subho</a> · SUB Brand · 2026</p>
