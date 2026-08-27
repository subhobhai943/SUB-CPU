/*
 * sub_cpu.h — SUB-CPU Architecture Header
 * Copyright (C) 2026 Subho (subhobhai943)
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SUB_CPU_H
#define SUB_CPU_H

#include <stdint.h>
#include <stddef.h>

/* Word size */
typedef uint16_t sub_word_t;
typedef uint8_t  sub_byte_t;

/* Memory size: 64 KB */
#define SUB_MEM_SIZE     0x10000
#define SUB_STACK_START  0xFFFF

/* Register count */
#define SUB_NUM_REGS     8

/* FLAGS bits */
#define FLAG_Z  (1 << 3)   /* Zero       */
#define FLAG_C  (1 << 2)   /* Carry      */
#define FLAG_N  (1 << 1)   /* Negative   */
#define FLAG_V  (1 << 0)   /* Overflow   */

/* Opcodes */
typedef enum {
    OP_ADD  = 0x0,
    OP_SUB  = 0x1,
    OP_AND  = 0x2,
    OP_OR   = 0x3,
    OP_XOR  = 0x4,
    OP_NOT  = 0x5,
    OP_SHL  = 0x6,
    OP_SHR  = 0x7,
    OP_LDI  = 0x8,
    OP_LDR  = 0x9,
    OP_STR  = 0xA,
    OP_MOV  = 0xB,
    OP_JMP  = 0xC,
    OP_JZ   = 0xD,
    OP_JNZ  = 0xE,
    OP_CALL = 0xF,
} sub_opcode_t;

/* CPU State */
typedef struct {
    sub_word_t regs[SUB_NUM_REGS];  /* R0–R7            */
    sub_word_t pc;                   /* Program Counter  */
    sub_word_t sp;                   /* Stack Pointer    */
    sub_word_t flags;                /* Status Flags     */
    sub_byte_t mem[SUB_MEM_SIZE];   /* 64 KB Memory     */
    int        halted;               /* Halt flag        */
} sub_cpu_t;

/* --- API --- */
void sub_cpu_init(sub_cpu_t *cpu);
void sub_cpu_load(sub_cpu_t *cpu, const sub_byte_t *program, size_t len, sub_word_t base);
void sub_cpu_step(sub_cpu_t *cpu);
void sub_cpu_run(sub_cpu_t *cpu);
void sub_cpu_dump(const sub_cpu_t *cpu);

#endif /* SUB_CPU_H */
