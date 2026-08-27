/*
 * sub64.h — SUB64 CPU Architecture Master Header
 * Copyright (C) 2026 Subho (subhobhai943)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * SUB64: An original 64-bit RISC/VLIW hybrid CPU architecture.
 */

#ifndef SUB64_H
#define SUB64_H

#include <stdint.h>
#include <stddef.h>

/* ── Word types ───────────────────────────────────────────── */
typedef uint64_t  sub64_word;   /* 64-bit general word        */
typedef uint32_t  sub64_instr;  /* 32-bit standard instruction */
typedef uint64_t  sub64_wide;   /* 64-bit WIDE instruction     */
typedef uint8_t   sub64_byte;

/* ── Constants ───────────────────────────────────────────── */
#define SUB64_NUM_GPR      24        /* X0–X23                  */
#define SUB64_NUM_VEC       8        /* V0–V7 (256-bit each)    */
#define SUB64_MEM_SIZE     (1ULL << 32)  /* 4 GB emulated RAM   */
#define SUB64_STACK_INIT   0xFFFF0000ULL
#define SUB64_VEC_LANES     4        /* 4 × 64-bit per vector   */

/* ── Register aliases ────────────────────────────────────── */
#define REG_ZERO   0   /* X0:  always zero          */
#define REG_RA     1   /* X1:  return address       */
#define REG_SP     2   /* X2:  stack pointer        */
#define REG_FP     3   /* X3:  frame pointer        */
#define REG_A0     4   /* X4:  arg0 / return val    */
#define REG_A1     5
#define REG_A2     6
#define REG_A3     7
#define REG_T0     8   /* X8–X15: temporaries       */
#define REG_S0    16   /* X16–X23: saved registers  */
#define REG_SYSCALL 8  /* X8: syscall number        */

/* ── FLAGS bits ──────────────────────────────────────────── */
#define FLAG_V   (1ULL << 0)   /* Overflow          */
#define FLAG_N   (1ULL << 1)   /* Negative          */
#define FLAG_C   (1ULL << 2)   /* Carry             */
#define FLAG_Z   (1ULL << 3)   /* Zero              */
#define FLAG_P   (1ULL << 4)   /* Parity            */
#define FLAG_IP  (1ULL << 5)   /* Interrupt Pending */

/* ── Privilege Rings ─────────────────────────────────────── */
typedef enum {
    RING_KERNEL = 0,
    RING_USER   = 1,
    RING_GUEST  = 2,
} sub64_ring;

/* ── Opcodes ─────────────────────────────────────────────── */
typedef enum {
    /* Arithmetic */
    OP_ADD   = 0x00, OP_ADDI  = 0x01,
    OP_SUB   = 0x02, OP_SUBI  = 0x03,
    OP_MUL   = 0x04, OP_MULH  = 0x05,
    OP_DIV   = 0x06, OP_REM   = 0x07,
    OP_NEG   = 0x08, OP_ABS   = 0x09,
    /* Logic */
    OP_AND   = 0x10, OP_ANDI  = 0x11,
    OP_OR    = 0x12, OP_ORI   = 0x13,
    OP_XOR   = 0x14, OP_NOT   = 0x15,
    OP_SHL   = 0x16, OP_SHR   = 0x17,
    OP_SAR   = 0x18, OP_ROL   = 0x19,
    OP_ROR   = 0x1A, OP_CLZ   = 0x1B,
    OP_CTZ   = 0x1C, OP_POPC  = 0x1D,
    OP_BEXT  = 0x1E, OP_BDEP  = 0x1F,
    /* Memory */
    OP_LD64  = 0x20, OP_LD32  = 0x21,
    OP_LD16  = 0x22, OP_LD8   = 0x23,
    OP_ST64  = 0x24, OP_ST32  = 0x25,
    OP_ST16  = 0x26, OP_ST8   = 0x27,
    OP_LDIM  = 0x28, OP_LEA   = 0x29,
    OP_PUSH  = 0x2A, OP_POP   = 0x2B,
    OP_XCHG  = 0x2C, OP_CAS   = 0x2D,
    /* Control flow */
    OP_JMP   = 0x30, OP_JMPI  = 0x31,
    OP_CALL  = 0x32, OP_CALLI = 0x33,
    OP_RET   = 0x34, OP_BEQ   = 0x35,
    OP_BNE   = 0x36, OP_BLT   = 0x37,
    OP_BGE   = 0x38, OP_BLTU  = 0x39,
    OP_BGEU  = 0x3A, OP_LOOP  = 0x3B,
    OP_SYSCALL=0x3C, OP_SYSRET= 0x3D,
    OP_FENCE = 0x3E, OP_HLT   = 0x3F,
    /* Compare / select */
    OP_CMP   = 0x40, OP_CMPI  = 0x41,
    OP_TST   = 0x42, OP_TSTI  = 0x43,
    OP_SEQ   = 0x44, OP_SNE   = 0x45,
    OP_SLT   = 0x46, OP_SLTU  = 0x47,
    /* Move */
    OP_MOV   = 0x50, OP_MOVZ  = 0x51,
    OP_MOVNZ = 0x52, OP_MOVC  = 0x53,
    OP_MOVN  = 0x54, OP_SEL   = 0x55,
    OP_ZEXT32= 0x56, OP_SEXT32= 0x57,
    OP_SEXT16= 0x58, OP_SEXT8 = 0x59,
} sub64_opcode;

/* ── Predicate codes ─────────────────────────────────────── */
typedef enum {
    PRED_AL = 0,  /* Always        */
    PRED_EQ = 1,  /* Z == 1        */
    PRED_NE = 2,  /* Z == 0        */
    PRED_CS = 3,  /* C == 1        */
} sub64_pred;

/* ── Vector register (256-bit = 4 × 64-bit) ─────────────── */
typedef struct {
    uint64_t lane[SUB64_VEC_LANES];
} sub64_vreg;

/* ── CPU State ───────────────────────────────────────────── */
typedef struct {
    sub64_word   gpr[SUB64_NUM_GPR]; /* X0–X23          */
    sub64_vreg   vec[SUB64_NUM_VEC]; /* V0–V7           */
    sub64_word   pc;                 /* Program Counter */
    sub64_word   sp;                 /* Stack Pointer   */
    sub64_word   lr;                 /* Link Register   */
    sub64_word   flags;              /* ZCNVIP flags    */
    sub64_word   kvec;               /* Kernel IVT base */
    sub64_word   uvec;               /* User trap base  */
    sub64_ring   ring;               /* Privilege ring  */
    int          halted;             /* Halt flag       */
    sub64_byte  *mem;                /* Heap-alloc RAM  */
    size_t       mem_size;
} sub64_cpu;

/* ── Instruction decode helpers ──────────────────────────── */
#define INSTR_OPCODE(i)  (((i) >> 26) & 0x3F)
#define INSTR_PRED(i)    (((i) >> 24) & 0x03)
#define INSTR_RD(i)      (((i) >> 19) & 0x1F)
#define INSTR_RA(i)      (((i) >> 14) & 0x1F)
#define INSTR_RB(i)      (((i) >>  9) & 0x1F)
#define INSTR_FUNC(i)    (((i) >>  6) & 0x07)
#define INSTR_IMM6(i)    ( (i)        & 0x3F)
#define INSTR_IS_WIDE(i) (((i) >> 31) & 0x01)

/* ── API ─────────────────────────────────────────────────── */
int       sub64_cpu_init(sub64_cpu *cpu, size_t mem_bytes);
void      sub64_cpu_free(sub64_cpu *cpu);
void      sub64_cpu_load(sub64_cpu *cpu, const sub64_byte *prog,
                         size_t len, sub64_word base);
int       sub64_cpu_step(sub64_cpu *cpu);
void      sub64_cpu_run(sub64_cpu *cpu);
void      sub64_cpu_dump(const sub64_cpu *cpu);

#endif /* SUB64_H */
