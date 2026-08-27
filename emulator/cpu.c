/*
 * cpu.c — SUB64 CPU Emulator Core
 * Copyright (C) 2026 Subho (subhobhai943)
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "../include/sub64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Memory helpers ──────────────────────────────────────── */
static inline uint64_t mem_read64(sub64_cpu *c, uint64_t addr) {
    if (addr + 8 > c->mem_size) return 0;
    uint64_t v; memcpy(&v, c->mem + addr, 8); return v;
}
static inline void mem_write64(sub64_cpu *c, uint64_t addr, uint64_t v) {
    if (addr + 8 > c->mem_size) return;
    memcpy(c->mem + addr, &v, 8);
}
static inline uint32_t mem_read32(sub64_cpu *c, uint64_t addr) {
    if (addr + 4 > c->mem_size) return 0;
    uint32_t v; memcpy(&v, c->mem + addr, 4); return v;
}
static inline uint8_t mem_read8(sub64_cpu *c, uint64_t addr) {
    return (addr < c->mem_size) ? c->mem[addr] : 0;
}

/* ── Register write (X0 is always zero) ──────────────────── */
static inline void wreg(sub64_cpu *c, int r, uint64_t v) {
    if (r != 0 && r < SUB64_NUM_GPR) c->gpr[r] = v;
}

/* ── Flag helpers ────────────────────────────────────────── */
static void update_flags(sub64_cpu *c, uint64_t result,
                          uint64_t a, uint64_t b, int is_sub) {
    c->flags &= ~(FLAG_Z | FLAG_N | FLAG_C | FLAG_V);
    if (result == 0)         c->flags |= FLAG_Z;
    if (result >> 63)        c->flags |= FLAG_N;
    if (is_sub) {
        if (a < b)           c->flags |= FLAG_C; /* borrow */
        if (((a ^ b) & 0x8000000000000000ULL) &&
            !((b ^ result) & 0x8000000000000000ULL))
            c->flags |= FLAG_V;
    } else {
        /* addition carry */
        if (result < a)      c->flags |= FLAG_C;
    }
}

/* ── Predicate check ─────────────────────────────────────── */
static int check_pred(sub64_cpu *c, int pred) {
    switch (pred) {
        case PRED_AL: return 1;
        case PRED_EQ: return !!(c->flags & FLAG_Z);
        case PRED_NE: return  !(c->flags & FLAG_Z);
        case PRED_CS: return !!(c->flags & FLAG_C);
    }
    return 1;
}

/* ── Execute one 32-bit instruction ─────────────────────── */
static void exec_instr(sub64_cpu *c, sub64_instr instr) {
    int op   = INSTR_OPCODE(instr);
    int pred = INSTR_PRED(instr);
    int rd   = INSTR_RD(instr);
    int ra   = INSTR_RA(instr);
    int rb   = INSTR_RB(instr);
    int imm6 = INSTR_IMM6(instr);

    if (!check_pred(c, pred)) return;

    uint64_t va = (ra < SUB64_NUM_GPR) ? c->gpr[ra] : 0;
    uint64_t vb = (rb < SUB64_NUM_GPR) ? c->gpr[rb] : 0;
    uint64_t result = 0;

    switch ((sub64_opcode)op) {
        /* ─── Arithmetic ─────────────────── */
        case OP_ADD:  result = va + vb; update_flags(c,result,va,vb,0); wreg(c,rd,result); break;
        case OP_ADDI: result = va + (uint64_t)imm6; update_flags(c,result,va,imm6,0); wreg(c,rd,result); break;
        case OP_SUB:  result = va - vb; update_flags(c,result,va,vb,1); wreg(c,rd,result); break;
        case OP_SUBI: result = va - (uint64_t)imm6; update_flags(c,result,va,imm6,1); wreg(c,rd,result); break;
        case OP_MUL:  wreg(c,rd, va * vb); break;
        case OP_MULH: wreg(c,rd, (uint64_t)((__uint128_t)va * vb >> 64)); break;
        case OP_DIV:  if(vb) wreg(c,rd,(int64_t)va/(int64_t)vb); break;
        case OP_REM:  if(vb) wreg(c,rd,(int64_t)va%(int64_t)vb); break;
        case OP_NEG:  wreg(c,rd, (uint64_t)(-(int64_t)va)); break;
        case OP_ABS:  wreg(c,rd, (int64_t)va<0 ? (uint64_t)(-(int64_t)va) : va); break;
        /* ─── Logic ──────────────────────── */
        case OP_AND:  wreg(c,rd, va & vb); break;
        case OP_ANDI: wreg(c,rd, va & (uint64_t)imm6); break;
        case OP_OR:   wreg(c,rd, va | vb); break;
        case OP_ORI:  wreg(c,rd, va | (uint64_t)imm6); break;
        case OP_XOR:  wreg(c,rd, va ^ vb); break;
        case OP_NOT:  wreg(c,rd, ~va); break;
        case OP_SHL:  wreg(c,rd, va << (vb & 63)); break;
        case OP_SHR:  wreg(c,rd, va >> (vb & 63)); break;
        case OP_SAR:  wreg(c,rd, (uint64_t)((int64_t)va >> (vb & 63))); break;
        case OP_ROL:  { uint64_t s=vb&63; wreg(c,rd,(va<<s)|(va>>(64-s))); break; }
        case OP_ROR:  { uint64_t s=vb&63; wreg(c,rd,(va>>s)|(va<<(64-s))); break; }
        case OP_CLZ:  wreg(c,rd, va ? (uint64_t)__builtin_clzll(va) : 64); break;
        case OP_CTZ:  wreg(c,rd, va ? (uint64_t)__builtin_ctzll(va) : 64); break;
        case OP_POPC: wreg(c,rd, (uint64_t)__builtin_popcountll(va)); break;
        /* ─── Memory ─────────────────────── */
        case OP_LD64: wreg(c,rd, mem_read64(c, va+(uint64_t)imm6)); break;
        case OP_LD32: wreg(c,rd, (uint64_t)mem_read32(c, va+(uint64_t)imm6)); break;
        case OP_LD8:  wreg(c,rd, (uint64_t)mem_read8(c,  va+(uint64_t)imm6)); break;
        case OP_ST64: mem_write64(c, va+(uint64_t)imm6, vb); break;
        case OP_PUSH:
            c->gpr[REG_SP] -= 8;
            mem_write64(c, c->gpr[REG_SP], va);
            break;
        case OP_POP:
            wreg(c, rd, mem_read64(c, c->gpr[REG_SP]));
            c->gpr[REG_SP] += 8;
            break;
        case OP_LDIM: {
            /* next 6 bytes = 48-bit immediate (little-endian) */
            uint64_t imm48 = 0;
            for (int i = 0; i < 6; i++)
                imm48 |= (uint64_t)mem_read8(c, c->pc + i) << (i * 8);
            c->pc += 6;
            wreg(c, rd, imm48);
            break;
        }
        case OP_LEA:  wreg(c,rd, va + (uint64_t)imm6); break;
        /* ─── Control ────────────────────── */
        case OP_JMP:   c->pc = va; break;
        case OP_JMPI:  c->pc = c->pc + (int64_t)(int8_t)imm6 * 4; break;
        case OP_CALL:  c->lr = c->pc; c->pc = va; break;
        case OP_CALLI: c->lr = c->pc; c->pc = c->pc + (int64_t)(int8_t)imm6 * 4; break;
        case OP_RET:   c->pc = c->lr; break;
        case OP_BEQ:   if(va==vb) c->pc += (int64_t)(int8_t)imm6*4; break;
        case OP_BNE:   if(va!=vb) c->pc += (int64_t)(int8_t)imm6*4; break;
        case OP_BLT:   if((int64_t)va<(int64_t)vb) c->pc += (int64_t)(int8_t)imm6*4; break;
        case OP_BGE:   if((int64_t)va>=(int64_t)vb) c->pc += (int64_t)(int8_t)imm6*4; break;
        case OP_LOOP:
            c->gpr[23]--;
            if (c->gpr[23] != 0) c->pc += (int64_t)(int8_t)imm6 * 4;
            break;
        case OP_SYSCALL:
            /* Basic syscalls for emulator */
            if (c->gpr[REG_SYSCALL] == 1) { /* sys_write stdout */
                uint64_t ptr = c->gpr[4];
                uint64_t len = c->gpr[5];
                for (uint64_t i = 0; i < len && ptr+i < c->mem_size; i++)
                    putchar(c->mem[ptr + i]);
            } else if (c->gpr[REG_SYSCALL] == 0) { /* sys_exit */
                c->halted = 1;
            }
            break;
        case OP_HLT:   c->halted = 1; break;
        /* ─── Compare ────────────────────── */
        case OP_CMP:   update_flags(c, va-vb, va, vb, 1); break;
        case OP_CMPI:  update_flags(c, va-(uint64_t)imm6, va, imm6, 1); break;
        case OP_TST:   { uint64_t r=va&vb; c->flags=(r==0?FLAG_Z:0)|(r>>63?FLAG_N:0); break; }
        case OP_SEQ:   wreg(c,rd, va==vb ? 1 : 0); break;
        case OP_SNE:   wreg(c,rd, va!=vb ? 1 : 0); break;
        case OP_SLT:   wreg(c,rd, (int64_t)va<(int64_t)vb ? 1 : 0); break;
        case OP_SLTU:  wreg(c,rd, va<vb ? 1 : 0); break;
        /* ─── Move ───────────────────────── */
        case OP_MOV:   wreg(c,rd, va); break;
        case OP_MOVZ:  if(c->flags & FLAG_Z)  wreg(c,rd, va); break;
        case OP_MOVNZ: if(!(c->flags & FLAG_Z)) wreg(c,rd, va); break;
        case OP_MOVC:  if(c->flags & FLAG_C)  wreg(c,rd, va); break;
        case OP_MOVN:  if(c->flags & FLAG_N)  wreg(c,rd, va); break;
        case OP_SEL:   wreg(c,rd, (c->flags & FLAG_Z) ? va : vb); break;
        case OP_SEXT32: wreg(c,rd, (uint64_t)(int64_t)(int32_t)va); break;
        case OP_SEXT16: wreg(c,rd, (uint64_t)(int64_t)(int16_t)va); break;
        case OP_SEXT8:  wreg(c,rd, (uint64_t)(int64_t)(int8_t)va); break;
        case OP_ZEXT32: wreg(c,rd, va & 0xFFFFFFFFULL); break;
        default: break;
    }
}

/* ── Public API ──────────────────────────────────────────── */
int sub64_cpu_init(sub64_cpu *cpu, size_t mem_bytes) {
    memset(cpu, 0, sizeof(sub64_cpu));
    cpu->mem = calloc(1, mem_bytes);
    if (!cpu->mem) return -1;
    cpu->mem_size = mem_bytes;
    cpu->gpr[REG_SP] = SUB64_STACK_INIT;
    cpu->ring = RING_USER;
    return 0;
}

void sub64_cpu_free(sub64_cpu *cpu) {
    free(cpu->mem);
    cpu->mem = NULL;
}

void sub64_cpu_load(sub64_cpu *cpu, const sub64_byte *prog,
                    size_t len, sub64_word base) {
    if (base + len > cpu->mem_size) len = cpu->mem_size - base;
    memcpy(cpu->mem + base, prog, len);
    cpu->pc = base;
}

int sub64_cpu_step(sub64_cpu *cpu) {
    if (cpu->halted) return 0;

    sub64_instr instr = mem_read32(cpu, cpu->pc);
    cpu->pc += 4;

    if (INSTR_IS_WIDE(instr)) {
        /* WIDE: 64-bit dual-issue — read second word */
        sub64_instr slot_a = instr & 0x7FFFFFFF;
        sub64_instr slot_b = mem_read32(cpu, cpu->pc);
        cpu->pc += 4;
        exec_instr(cpu, slot_a);
        exec_instr(cpu, slot_b & 0x7FFFFFFF);
    } else {
        exec_instr(cpu, instr);
    }
    return 1;
}

void sub64_cpu_run(sub64_cpu *cpu) {
    while (!cpu->halted)
        sub64_cpu_step(cpu);
}

void sub64_cpu_dump(const sub64_cpu *cpu) {
    printf("\n╔══════════════ SUB64 CPU Dump ══════════════╗\n");
    for (int i = 0; i < SUB64_NUM_GPR; i++)
        printf("║  X%-2d = 0x%016llX  (%llu)\n",
            i, (unsigned long long)cpu->gpr[i],
               (unsigned long long)cpu->gpr[i]);
    printf("║  PC   = 0x%016llX\n", (unsigned long long)cpu->pc);
    printf("║  SP   = 0x%016llX\n", (unsigned long long)cpu->gpr[REG_SP]);
    printf("║  LR   = 0x%016llX\n", (unsigned long long)cpu->lr);
    printf("║  FLAGS= Z:%d C:%d N:%d V:%d P:%d IP:%d\n",
        !!(cpu->flags & FLAG_Z), !!(cpu->flags & FLAG_C),
        !!(cpu->flags & FLAG_N), !!(cpu->flags & FLAG_V),
        !!(cpu->flags & FLAG_P), !!(cpu->flags & FLAG_IP));
    printf("║  RING = %d (%s)\n", cpu->ring,
        cpu->ring==0?"Kernel":cpu->ring==1?"User":"Guest");
    printf("╚════════════════════════════════════════════╝\n");
}
