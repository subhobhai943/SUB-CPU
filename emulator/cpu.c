/*
 * cpu.c — SUB-CPU Emulator Core
 * Copyright (C) 2026 Subho (subhobhai943)
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "../include/sub_cpu.h"
#include <stdio.h>
#include <string.h>

static void set_flags(sub_cpu_t *cpu, uint32_t result, sub_word_t a, sub_word_t b) {
    cpu->flags = 0;
    if ((result & 0xFFFF) == 0)       cpu->flags |= FLAG_Z;
    if (result > 0xFFFF)              cpu->flags |= FLAG_C;
    if (result & 0x8000)              cpu->flags |= FLAG_N;
    /* Overflow: sign of a and b same, but result sign differs */
    if (!((a ^ b) & 0x8000) && ((a ^ result) & 0x8000))
        cpu->flags |= FLAG_V;
}

void sub_cpu_init(sub_cpu_t *cpu) {
    memset(cpu, 0, sizeof(sub_cpu_t));
    cpu->sp = SUB_STACK_START;
}

void sub_cpu_load(sub_cpu_t *cpu, const sub_byte_t *prog, size_t len, sub_word_t base) {
    if (len > SUB_MEM_SIZE - base) len = SUB_MEM_SIZE - base;
    memcpy(cpu->mem + base, prog, len);
    cpu->pc = base;
}

void sub_cpu_step(sub_cpu_t *cpu) {
    if (cpu->halted) return;

    sub_word_t instr = (cpu->mem[cpu->pc] << 8) | cpu->mem[cpu->pc + 1];
    cpu->pc += 2;

    uint8_t opcode = (instr >> 12) & 0xF;
    uint8_t rd     = (instr >>  9) & 0x7;
    uint8_t ra     = (instr >>  6) & 0x7;
    uint8_t rb     = (instr >>  3) & 0x7;
    uint8_t mode   =  instr        & 0x7;

    sub_word_t imm = 0;
    if (opcode == OP_LDI || opcode == OP_JMP ||
        opcode == OP_JZ  || opcode == OP_JNZ ||
        opcode == OP_CALL) {
        imm = (cpu->mem[cpu->pc] << 8) | cpu->mem[cpu->pc + 1];
        cpu->pc += 2;
    }

    uint32_t result;

    switch ((sub_opcode_t)opcode) {
        case OP_ADD:
            result = (uint32_t)cpu->regs[ra] + cpu->regs[rb];
            set_flags(cpu, result, cpu->regs[ra], cpu->regs[rb]);
            cpu->regs[rd] = (sub_word_t)result;
            break;
        case OP_SUB:
            result = (uint32_t)cpu->regs[ra] - cpu->regs[rb];
            set_flags(cpu, result, cpu->regs[ra], cpu->regs[rb]);
            cpu->regs[rd] = (sub_word_t)result;
            break;
        case OP_AND:
            cpu->regs[rd] = cpu->regs[ra] & cpu->regs[rb];
            set_flags(cpu, cpu->regs[rd], cpu->regs[ra], cpu->regs[rb]);
            break;
        case OP_OR:
            cpu->regs[rd] = cpu->regs[ra] | cpu->regs[rb];
            set_flags(cpu, cpu->regs[rd], cpu->regs[ra], cpu->regs[rb]);
            break;
        case OP_XOR:
            cpu->regs[rd] = cpu->regs[ra] ^ cpu->regs[rb];
            set_flags(cpu, cpu->regs[rd], cpu->regs[ra], cpu->regs[rb]);
            break;
        case OP_NOT:
            cpu->regs[rd] = ~cpu->regs[ra];
            set_flags(cpu, cpu->regs[rd], cpu->regs[ra], 0);
            break;
        case OP_SHL:
            cpu->regs[rd] = cpu->regs[ra] << cpu->regs[rb];
            break;
        case OP_SHR:
            cpu->regs[rd] = cpu->regs[ra] >> cpu->regs[rb];
            break;
        case OP_LDI:
            cpu->regs[rd] = imm;
            break;
        case OP_LDR:
            cpu->regs[rd] = (cpu->mem[cpu->regs[ra]] << 8) |
                             cpu->mem[cpu->regs[ra] + 1];
            break;
        case OP_STR:
            cpu->mem[cpu->regs[ra]]     = (cpu->regs[rb] >> 8) & 0xFF;
            cpu->mem[cpu->regs[ra] + 1] =  cpu->regs[rb]       & 0xFF;
            break;
        case OP_MOV:
            cpu->regs[rd] = cpu->regs[ra];
            break;
        case OP_JMP:
            cpu->pc = imm;
            break;
        case OP_JZ:
            if (cpu->flags & FLAG_Z) cpu->pc = imm;
            break;
        case OP_JNZ:
            if (!(cpu->flags & FLAG_Z)) cpu->pc = imm;
            break;
        case OP_CALL:
            cpu->sp -= 2;
            cpu->mem[cpu->sp]     = (cpu->pc >> 8) & 0xFF;
            cpu->mem[cpu->sp + 1] =  cpu->pc       & 0xFF;
            cpu->pc = imm;
            break;
        default:
            /* Extended ops via mode bits: PUSH/POP/RET/NOP/HLT */
            if (mode == 0x1) { /* PUSH */
                cpu->sp -= 2;
                cpu->mem[cpu->sp]     = (cpu->regs[rd] >> 8) & 0xFF;
                cpu->mem[cpu->sp + 1] =  cpu->regs[rd]       & 0xFF;
            } else if (mode == 0x2) { /* POP */
                cpu->regs[rd] = (cpu->mem[cpu->sp] << 8) | cpu->mem[cpu->sp + 1];
                cpu->sp += 2;
            } else if (mode == 0x3) { /* RET */
                cpu->pc = (cpu->mem[cpu->sp] << 8) | cpu->mem[cpu->sp + 1];
                cpu->sp += 2;
            } else if (mode == 0x7) { /* HLT */
                cpu->halted = 1;
            }
            break;
    }
}

void sub_cpu_run(sub_cpu_t *cpu) {
    while (!cpu->halted)
        sub_cpu_step(cpu);
}

void sub_cpu_dump(const sub_cpu_t *cpu) {
    printf("\n=== SUB-CPU State Dump ===\n");
    for (int i = 0; i < SUB_NUM_REGS; i++)
        printf("  R%d = 0x%04X (%u)\n", i, cpu->regs[i], cpu->regs[i]);
    printf("  PC    = 0x%04X\n", cpu->pc);
    printf("  SP    = 0x%04X\n", cpu->sp);
    printf("  FLAGS = Z:%d C:%d N:%d V:%d\n",
        !!(cpu->flags & FLAG_Z),
        !!(cpu->flags & FLAG_C),
        !!(cpu->flags & FLAG_N),
        !!(cpu->flags & FLAG_V));
    printf("=========================\n");
}
