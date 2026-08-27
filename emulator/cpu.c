/*
 * cpu.c — SUB64 Core CPU Emulation Engine
 * Copyright (C) 2026 Subho (subhobhai943)
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "sub64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern void sub64_vpu_execute(sub64_cpu *cpu, uint32_t opcode, int vd, int va, int vb, int rd, int ra);
extern void sub64_fpu_execute(sub64_cpu *cpu, uint32_t opcode, int rd, int ra, int rb);

int sub64_cpu_init(sub64_cpu *cpu, size_t mem_bytes) {
    memset(cpu, 0, sizeof(*cpu));
    cpu->mem = (sub64_byte *)calloc(1, mem_bytes);
    if (!cpu->mem) return -1;
    cpu->mem_size = mem_bytes;
    cpu->sp = SUB64_STACK_INIT;
    cpu->gpr[REG_SP] = SUB64_STACK_INIT;
    cpu->ring = RING_KERNEL;
    cpu->halted = 0;
    return 0;
}

void sub64_cpu_free(sub64_cpu *cpu) {
    if (cpu->mem) {
        free(cpu->mem);
        cpu->mem = NULL;
    }
}

void sub64_cpu_load(sub64_cpu *cpu, const sub64_byte *prog, size_t len, sub64_word base) {
    if (base + len <= cpu->mem_size) {
        memcpy(cpu->mem + base, prog, len);
    }
    cpu->pc = base;
}

void sub64_cpu_trap(sub64_cpu *cpu, int vector, uint64_t error_code) {
    /* Faults restart the instruction; traps/syscalls resume at next instruction */
    if (vector == VEC_SYSCALL || vector == VEC_ARITH_TRAP || vector >= VEC_SOFT_BASE) {
        cpu->uvec = cpu->pc + 4;
    } else {
        cpu->uvec = cpu->pc;
    }
    cpu->ring = RING_KERNEL;
    cpu->gpr[REG_A0] = error_code;

    if (cpu->kvec != 0) {
        cpu->pc = cpu->kvec + (uint64_t)(vector * 16);
    } else {
        /* No IVT registered — halt CPU with vector in A0 */
        cpu->gpr[REG_A0] = (uint64_t)vector;
        cpu->halted = 1;
    }
}

static inline uint8_t mem_read8(sub64_cpu *cpu, sub64_word addr) {
    if (addr >= cpu->mem_size) {
        sub64_cpu_trap(cpu, VEC_PAGE_FAULT_R, addr);
        return 0;
    }
    return cpu->mem[addr];
}

static inline uint16_t mem_read16(sub64_cpu *cpu, sub64_word addr) {
    if (addr + 1 >= cpu->mem_size) {
        sub64_cpu_trap(cpu, VEC_PAGE_FAULT_R, addr);
        return 0;
    }
    return (uint16_t)cpu->mem[addr] | ((uint16_t)cpu->mem[addr + 1] << 8);
}

static inline uint32_t mem_read32(sub64_cpu *cpu, sub64_word addr) {
    if (addr + 3 >= cpu->mem_size) {
        sub64_cpu_trap(cpu, VEC_PAGE_FAULT_R, addr);
        return 0;
    }
    return (uint32_t)cpu->mem[addr] |
           ((uint32_t)cpu->mem[addr + 1] << 8) |
           ((uint32_t)cpu->mem[addr + 2] << 16) |
           ((uint32_t)cpu->mem[addr + 3] << 24);
}

static inline uint64_t mem_read64(sub64_cpu *cpu, sub64_word addr) {
    if (addr + 7 >= cpu->mem_size) {
        sub64_cpu_trap(cpu, VEC_PAGE_FAULT_R, addr);
        return 0;
    }
    uint64_t val = 0;
    for (int i = 0; i < 8; i++)
        val |= ((uint64_t)cpu->mem[addr + i]) << (i * 8);
    return val;
}

static inline void mem_write8(sub64_cpu *cpu, sub64_word addr, uint8_t val) {
    if (addr >= cpu->mem_size) {
        sub64_cpu_trap(cpu, VEC_PAGE_FAULT_W, addr);
        return;
    }
    cpu->mem[addr] = val;
}

static inline void mem_write16(sub64_cpu *cpu, sub64_word addr, uint16_t val) {
    if (addr + 1 >= cpu->mem_size) {
        sub64_cpu_trap(cpu, VEC_PAGE_FAULT_W, addr);
        return;
    }
    cpu->mem[addr]     = val & 0xFF;
    cpu->mem[addr + 1] = (val >> 8) & 0xFF;
}

static inline void mem_write32(sub64_cpu *cpu, sub64_word addr, uint32_t val) {
    if (addr + 3 >= cpu->mem_size) {
        sub64_cpu_trap(cpu, VEC_PAGE_FAULT_W, addr);
        return;
    }
    cpu->mem[addr]     = val & 0xFF;
    cpu->mem[addr + 1] = (val >> 8) & 0xFF;
    cpu->mem[addr + 2] = (val >> 16) & 0xFF;
    cpu->mem[addr + 3] = (val >> 24) & 0xFF;
}

static inline void mem_write64(sub64_cpu *cpu, sub64_word addr, uint64_t val) {
    if (addr + 7 >= cpu->mem_size) {
        sub64_cpu_trap(cpu, VEC_PAGE_FAULT_W, addr);
        return;
    }
    for (int i = 0; i < 8; i++)
        cpu->mem[addr + i] = (uint8_t)((val >> (i * 8)) & 0xFF);
}

static void update_flags(sub64_cpu *cpu, uint64_t result, uint64_t a, uint64_t b, int is_sub) {
    cpu->flags &= ~(FLAG_Z | FLAG_N | FLAG_C | FLAG_V | FLAG_P);

    if (result == 0) cpu->flags |= FLAG_Z;
    if (result & (1ULL << 63)) cpu->flags |= FLAG_N;

    if (is_sub) {
        if (a < b) cpu->flags |= FLAG_C;
        if (((a ^ b) & (a ^ result)) >> 63) cpu->flags |= FLAG_V;
    } else {
        if (result < a) cpu->flags |= FLAG_C;
        if ((~(a ^ b) & (a ^ result)) >> 63) cpu->flags |= FLAG_V;
    }

    if (__builtin_parityll(result & 0xFF) == 0) cpu->flags |= FLAG_P;
}

static int check_pred(const sub64_cpu *cpu, uint32_t pred) {
    switch (pred) {
    case PRED_AL: return 1;
    case PRED_EQ: return (cpu->flags & FLAG_Z) != 0;
    case PRED_NE: return (cpu->flags & FLAG_Z) == 0;
    case PRED_CS: return (cpu->flags & FLAG_C) != 0;
    default:      return 1;
    }
}

static void execute_inst(sub64_cpu *cpu, uint32_t inst, uint64_t imm48, int *advance_pc) {
    uint32_t pred = INSTR_PRED(inst);
    if (!check_pred(cpu, pred)) return;

    uint32_t op   = INSTR_OPCODE(inst);
    uint32_t rd   = INSTR_RD(inst);
    uint32_t ra   = INSTR_RA(inst);
    uint32_t rb   = INSTR_RB(inst);
    uint32_t imm6 = INSTR_IMM6(inst);

    uint64_t val_a = (ra < SUB64_NUM_GPR) ? cpu->gpr[ra] : 0;
    uint64_t val_b = (rb < SUB64_NUM_GPR) ? cpu->gpr[rb] : 0;
    int64_t simm6 = (int64_t)((int32_t)(imm6 << 26) >> 26);

    /* Safe register write macro — silently ignores writes to X0 */
    #define SET_GPR(r, val) do { if ((r) > 0 && (r) < SUB64_NUM_GPR) cpu->gpr[(r)] = (val); } while (0)

    /* Vector instructions */
    if (op >= OP_VADD64 && op <= OP_VBCAST) {
        sub64_vpu_execute(cpu, op, rd, ra, rb, rd, ra);
        return;
    }

    /* Floating-point instructions */
    if (op >= OP_FADD && op <= OP_FCVTFI) {
        sub64_fpu_execute(cpu, op, rd, ra, rb);
        return;
    }

    switch (op) {
    /* ── Arithmetic ───────────────────────────────────────── */
    case OP_ADD: {
        uint64_t res = val_a + val_b;
        update_flags(cpu, res, val_a, val_b, 0);
        SET_GPR(rd, res);
        break;
    }
    case OP_ADDI: {
        uint64_t res = val_a + (uint64_t)simm6;
        update_flags(cpu, res, val_a, (uint64_t)simm6, 0);
        SET_GPR(rd, res);
        break;
    }
    case OP_SUB: {
        uint64_t res = val_a - val_b;
        update_flags(cpu, res, val_a, val_b, 1);
        SET_GPR(rd, res);
        break;
    }
    case OP_SUBI: {
        uint64_t res = val_a - (uint64_t)simm6;
        update_flags(cpu, res, val_a, (uint64_t)simm6, 1);
        SET_GPR(rd, res);
        break;
    }
    case OP_MUL:
        SET_GPR(rd, val_a * val_b);
        break;
    case OP_MULH: {
        unsigned __int128 full = (unsigned __int128)val_a * (unsigned __int128)val_b;
        SET_GPR(rd, (uint64_t)(full >> 64));
        break;
    }
    case OP_DIV:
        if (val_b == 0) {
            *advance_pc = 0;
            sub64_cpu_trap(cpu, VEC_ARITH_TRAP, 0);
            break;
        }
        SET_GPR(rd, (uint64_t)((int64_t)val_a / (int64_t)val_b));
        break;
    case OP_REM:
        if (val_b == 0) {
            *advance_pc = 0;
            sub64_cpu_trap(cpu, VEC_ARITH_TRAP, 0);
            break;
        }
        SET_GPR(rd, (uint64_t)((int64_t)val_a % (int64_t)val_b));
        break;
    case OP_NEG: {
        uint64_t res = (uint64_t)(-(int64_t)val_a);
        update_flags(cpu, res, 0, val_a, 1);
        SET_GPR(rd, res);
        break;
    }
    case OP_ABS:
        SET_GPR(rd, ((int64_t)val_a < 0) ? (uint64_t)(-(int64_t)val_a) : val_a);
        break;

    /* ── Logic & Bit ──────────────────────────────────────── */
    case OP_AND:  SET_GPR(rd, val_a & val_b); break;
    case OP_ANDI: SET_GPR(rd, val_a & imm6); break;
    case OP_OR:   SET_GPR(rd, val_a | val_b); break;
    case OP_ORI:  SET_GPR(rd, val_a | imm6); break;
    case OP_XOR:  SET_GPR(rd, val_a ^ val_b); break;
    case OP_NOT:  SET_GPR(rd, ~val_a); break;
    case OP_SHL:  SET_GPR(rd, val_a << (val_b & 63)); break;
    case OP_SHR:  SET_GPR(rd, val_a >> (val_b & 63)); break;
    case OP_SAR:  SET_GPR(rd, (uint64_t)((int64_t)val_a >> (int)(val_b & 63))); break;
    case OP_ROL: {
        unsigned s = (unsigned)(val_b & 63);
        SET_GPR(rd, (val_a << s) | (val_a >> ((64 - s) & 63)));
        break;
    }
    case OP_ROR: {
        unsigned s = (unsigned)(val_b & 63);
        SET_GPR(rd, (val_a >> s) | (val_a << ((64 - s) & 63)));
        break;
    }
    case OP_CLZ:  SET_GPR(rd, val_a ? (uint64_t)__builtin_clzll(val_a) : 64); break;
    case OP_CTZ:  SET_GPR(rd, val_a ? (uint64_t)__builtin_ctzll(val_a) : 64); break;
    case OP_POPC: SET_GPR(rd, (uint64_t)__builtin_popcountll(val_a)); break;
    case OP_BEXT: SET_GPR(rd, (val_a >> (val_b & 63)) & 1); break;
    case OP_BDEP: SET_GPR(rd, (val_a & 1) << (val_b & 63)); break;

    /* ── Memory ───────────────────────────────────────────── */
    case OP_LD64: SET_GPR(rd, mem_read64(cpu, val_a + (uint64_t)simm6)); break;
    case OP_LD32: SET_GPR(rd, (uint64_t)mem_read32(cpu, val_a + (uint64_t)simm6)); break;
    case OP_LD16: SET_GPR(rd, (uint64_t)mem_read16(cpu, val_a + (uint64_t)simm6)); break;
    case OP_LD8:  SET_GPR(rd, (uint64_t)mem_read8(cpu, val_a + (uint64_t)simm6)); break;
    case OP_ST64: mem_write64(cpu, val_a + (uint64_t)simm6, val_b); break;
    case OP_ST32: mem_write32(cpu, val_a + (uint64_t)simm6, (uint32_t)val_b); break;
    case OP_ST16: mem_write16(cpu, val_a + (uint64_t)simm6, (uint16_t)val_b); break;
    case OP_ST8:  mem_write8(cpu, val_a + (uint64_t)simm6, (uint8_t)val_b); break;

    case OP_LDIM: SET_GPR(rd, imm48); break;
    case OP_LEA:  SET_GPR(rd, val_a + (uint64_t)simm6); break;

    case OP_PUSH:
        cpu->gpr[REG_SP] -= 8;
        cpu->sp = cpu->gpr[REG_SP];
        mem_write64(cpu, cpu->gpr[REG_SP], val_a);
        break;
    case OP_POP:
        SET_GPR(ra, mem_read64(cpu, cpu->gpr[REG_SP]));
        cpu->gpr[REG_SP] += 8;
        cpu->sp = cpu->gpr[REG_SP];
        break;
    case OP_XCHG: {
        uint64_t tmp = mem_read64(cpu, val_a);
        mem_write64(cpu, val_a, val_b);
        SET_GPR(rd, tmp);
        break;
    }
    case OP_CAS: {
        uint64_t cur = mem_read64(cpu, val_a);
        if (cur == val_b)
            mem_write64(cpu, val_a, cpu->gpr[rd]);
        SET_GPR(rd, cur);
        break;
    }

    /* ── Control Flow ─────────────────────────────────────── */
    case OP_JMP:
        *advance_pc = 0;
        cpu->pc = val_a;
        break;
    case OP_JMPI: {
        *advance_pc = 0;
        int32_t rel = (int32_t)((ra << 5) | rb);
        rel = (rel << 22) >> 22; /* 10-bit sign extend */
        cpu->pc += (int64_t)rel << 2;
        break;
    }
    case OP_CALL:
        *advance_pc = 0;
        cpu->gpr[REG_RA] = cpu->pc + 4;
        cpu->lr = cpu->pc + 4;
        cpu->pc = val_a;
        break;
    case OP_CALLI: {
        *advance_pc = 0;
        int32_t rel = (int32_t)((ra << 5) | rb);
        rel = (rel << 22) >> 22;
        cpu->gpr[REG_RA] = cpu->pc + 4;
        cpu->lr = cpu->pc + 4;
        cpu->pc += (int64_t)rel << 2;
        break;
    }
    case OP_RET:
        *advance_pc = 0;
        cpu->pc = cpu->gpr[REG_RA];
        break;
    case OP_BEQ:
        if (val_a == val_b)           { *advance_pc = 0; cpu->pc += simm6 << 2; }
        break;
    case OP_BNE:
        if (val_a != val_b)           { *advance_pc = 0; cpu->pc += simm6 << 2; }
        break;
    case OP_BLT:
        if ((int64_t)val_a < (int64_t)val_b)  { *advance_pc = 0; cpu->pc += simm6 << 2; }
        break;
    case OP_BGE:
        if ((int64_t)val_a >= (int64_t)val_b) { *advance_pc = 0; cpu->pc += simm6 << 2; }
        break;
    case OP_BLTU:
        if (val_a < val_b)            { *advance_pc = 0; cpu->pc += simm6 << 2; }
        break;
    case OP_BGEU:
        if (val_a >= val_b)           { *advance_pc = 0; cpu->pc += simm6 << 2; }
        break;
    case OP_LOOP: {
        cpu->gpr[23]--;
        if (cpu->gpr[23] != 0) {
            *advance_pc = 0;
            int32_t off11 = (int32_t)((rb << 6) | imm6);
            off11 = (off11 << 21) >> 21; /* 11-bit sign extend */
            cpu->pc += (int64_t)off11 << 2;
        }
        break;
    }

    /* ── System & Privilege ───────────────────────────────── */
    case OP_SYSCALL: {
        if (cpu->kvec != 0) {
            *advance_pc = 0;
            sub64_cpu_trap(cpu, VEC_SYSCALL, 0);
            break;
        }

        /* Direct Host Syscall Emulation Mode (bare-metal fallback) */
        uint64_t sys_num = cpu->gpr[REG_SYSCALL];
        if (sys_num == 0) {
            /* sys_exit(code) */
            cpu->halted = 1;
        } else if (sys_num == 1) {
            /* sys_write(fd, buf, len) */
            uint64_t fd  = cpu->gpr[REG_A0];
            uint64_t buf = cpu->gpr[REG_A1];
            uint64_t len = cpu->gpr[REG_A2];
            if (fd == 1 || fd == 2) {
                for (uint64_t i = 0; i < len; i++) {
                    char c = (char)mem_read8(cpu, buf + i);
                    ssize_t w = write((int)fd, &c, 1);
                    (void)w;
                }
                SET_GPR(REG_A0, len);
                SET_GPR(REG_A1, 0); /* err = 0 */
            } else {
                SET_GPR(REG_A0, (uint64_t)-1);
                SET_GPR(REG_A1, 1);
            }
        } else if (sys_num == 2) {
            /* sys_read(fd, buf, len) */
            uint64_t fd  = cpu->gpr[REG_A0];
            uint64_t buf = cpu->gpr[REG_A1];
            uint64_t len = cpu->gpr[REG_A2];
            if (fd == 0) {
                uint64_t read_bytes = 0;
                for (uint64_t i = 0; i < len; i++) {
                    char c = 0;
                    ssize_t r = read(0, &c, 1);
                    if (r <= 0) break;
                    mem_write8(cpu, buf + i, (uint8_t)c);
                    read_bytes++;
                }
                SET_GPR(REG_A0, read_bytes);
                SET_GPR(REG_A1, 0);
            } else {
                SET_GPR(REG_A0, (uint64_t)-1);
                SET_GPR(REG_A1, 1);
            }
        }
        break;
    }
    case OP_SYSRET:
        if (cpu->ring != RING_KERNEL) {
            *advance_pc = 0;
            sub64_cpu_trap(cpu, VEC_ILLEGAL_INST, 0);
            break;
        }
        *advance_pc = 0;
        cpu->pc = cpu->uvec;
        cpu->ring = RING_USER;
        break;

    case OP_FENCE:
        break; /* single-threaded memory model */

    case OP_HLT:
        cpu->halted = 1;
        break;

    /* ── Special Registers (RSR / WSR) ────────────────────── */
    case OP_RSR: {
        /* Rd = SREG[ra] */
        uint32_t sreg = ra;
        uint64_t val = 0;
        switch (sreg) {
        case SREG_FLAGS: val = cpu->flags; break;
        case SREG_KVEC:  val = cpu->kvec; break;
        case SREG_UVEC:  val = cpu->uvec; break;
        case SREG_RING:  val = (uint64_t)cpu->ring; break;
        case SREG_LR:    val = cpu->lr; break;
        case SREG_PC:    val = cpu->pc; break;
        default: break;
        }
        SET_GPR(rd, val);
        break;
    }
    case OP_WSR: {
        /* SREG[rd] = Ra (Ring 0 only) */
        if (cpu->ring != RING_KERNEL) {
            *advance_pc = 0;
            sub64_cpu_trap(cpu, VEC_ILLEGAL_INST, 0);
            break;
        }
        uint32_t sreg = rd;
        switch (sreg) {
        case SREG_FLAGS: cpu->flags = val_a; break;
        case SREG_KVEC:  cpu->kvec = val_a; break;
        case SREG_UVEC:  cpu->uvec = val_a; break;
        case SREG_RING:  cpu->ring = (sub64_ring)(val_a & 3); break;
        case SREG_LR:    cpu->lr = val_a; cpu->gpr[REG_RA] = val_a; break;
        default: break;
        }
        break;
    }
    case OP_INT: {
        /* Software interrupt: vector in imm6 */
        *advance_pc = 0;
        sub64_cpu_trap(cpu, VEC_SOFT_BASE + (int)imm6, 0);
        break;
    }

    /* ── Compare ──────────────────────────────────────────── */
    case OP_CMP: {
        uint64_t res = val_a - val_b;
        update_flags(cpu, res, val_a, val_b, 1);
        break;
    }
    case OP_CMPI: {
        uint64_t res = val_a - (uint64_t)simm6;
        update_flags(cpu, res, val_a, (uint64_t)simm6, 1);
        break;
    }
    case OP_TST: {
        uint64_t res = val_a & val_b;
        cpu->flags &= ~(FLAG_Z | FLAG_N);
        if (res == 0)           cpu->flags |= FLAG_Z;
        if (res & (1ULL << 63)) cpu->flags |= FLAG_N;
        break;
    }
    case OP_TSTI: {
        uint64_t res = val_a & imm6;
        cpu->flags &= ~(FLAG_Z | FLAG_N);
        if (res == 0)           cpu->flags |= FLAG_Z;
        if (res & (1ULL << 63)) cpu->flags |= FLAG_N;
        break;
    }
    case OP_SEQ:  SET_GPR(rd, (val_a == val_b) ? 1 : 0); break;
    case OP_SNE:  SET_GPR(rd, (val_a != val_b) ? 1 : 0); break;
    case OP_SLT:  SET_GPR(rd, ((int64_t)val_a < (int64_t)val_b) ? 1 : 0); break;
    case OP_SLTU: SET_GPR(rd, (val_a < val_b) ? 1 : 0); break;

    /* ── Move & Select ────────────────────────────────────── */
    case OP_MOV:   SET_GPR(rd, val_a); break;
    case OP_MOVZ:  if (cpu->flags & FLAG_Z)    SET_GPR(rd, val_a); break;
    case OP_MOVNZ: if (!(cpu->flags & FLAG_Z)) SET_GPR(rd, val_a); break;
    case OP_MOVC:  if (cpu->flags & FLAG_C)    SET_GPR(rd, val_a); break;
    case OP_MOVN:  if (cpu->flags & FLAG_N)    SET_GPR(rd, val_a); break;
    case OP_SEL:   SET_GPR(rd, (cpu->flags & FLAG_Z) ? val_a : val_b); break;

    case OP_ZEXT32: SET_GPR(rd, val_a & 0xFFFFFFFFULL); break;
    case OP_SEXT32: SET_GPR(rd, (uint64_t)(int64_t)(int32_t)(uint32_t)val_a); break;
    case OP_SEXT16: SET_GPR(rd, (uint64_t)(int64_t)(int16_t)(uint16_t)val_a); break;
    case OP_SEXT8:  SET_GPR(rd, (uint64_t)(int64_t)(int8_t)(uint8_t)val_a); break;

    default:
        fprintf(stderr, "SUB64: Illegal opcode 0x%02x at PC=0x%lx\n",
                op, (unsigned long)cpu->pc);
        *advance_pc = 0;
        sub64_cpu_trap(cpu, VEC_ILLEGAL_INST, op);
        break;
    }

    #undef SET_GPR
}

int sub64_cpu_step(sub64_cpu *cpu) {
    if (cpu->halted) return 1;

    uint32_t inst1 = mem_read32(cpu, cpu->pc);
    int is_wide = INSTR_IS_WIDE(inst1);

    int advance_pc = 4;

    if (is_wide) {
        /* Dual-issue WIDE instruction (64 bits) */
        uint64_t inst64 = mem_read64(cpu, cpu->pc);
        uint32_t slot_a = (uint32_t)((inst64 >> 32) & 0x7FFFFFFF);
        uint32_t slot_b = (uint32_t)(inst64 & 0x7FFFFFFF);

        int adv1 = 8, adv2 = 8;
        execute_inst(cpu, slot_a, 0, &adv1);
        cpu->gpr[REG_ZERO] = 0; /* X0 hardwired to zero */
        execute_inst(cpu, slot_b, 0, &adv2);
        cpu->gpr[REG_ZERO] = 0;

        advance_pc = (adv1 == 0 || adv2 == 0) ? 0 : 8;
    } else {
        uint32_t op = INSTR_OPCODE(inst1);
        uint64_t imm48 = 0;
        if (op == OP_LDIM) {
            /* 48-bit immediate from next 6 bytes (little-endian) */
            uint64_t lo = mem_read32(cpu, cpu->pc + 4);
            uint64_t hi = mem_read16(cpu, cpu->pc + 8);
            imm48 = lo | (hi << 32);
            advance_pc = 10;
        }
        int adv = advance_pc;
        execute_inst(cpu, inst1, imm48, &adv);
        cpu->gpr[REG_ZERO] = 0;
        advance_pc = adv;
    }

    if (advance_pc > 0) cpu->pc += (uint64_t)advance_pc;

    /* Keep architectural SP and LR in sync with GPR aliases */
    cpu->sp = cpu->gpr[REG_SP];
    cpu->lr = cpu->gpr[REG_RA];

    return cpu->halted;
}

void sub64_cpu_run(sub64_cpu *cpu) {
    while (!cpu->halted)
        sub64_cpu_step(cpu);
}

void sub64_cpu_dump(const sub64_cpu *cpu) {
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║                    SUB64 CPU STATE DUMP                   ║\n");
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    for (int i = 0; i < SUB64_NUM_GPR; i++) {
        printf("║  X%02d: 0x%016lx%s", i, (unsigned long)cpu->gpr[i],
               (i % 2 == 1) ? "  ║\n" : "  ");
    }
    printf("╟───────────────────────────────────────────────────────────╢\n");
    printf("║  PC : 0x%016lx    SP : 0x%016lx   ║\n",
           (unsigned long)cpu->pc, (unsigned long)cpu->sp);
    printf("║  LR : 0x%016lx    FLG: 0x%016lx   ║\n",
           (unsigned long)cpu->lr, (unsigned long)cpu->flags);
    printf("║  KVEC: 0x%016lx  UVEC: 0x%016lx  ║\n",
           (unsigned long)cpu->kvec, (unsigned long)cpu->uvec);
    printf("║  FLAGS: [%c%c%c%c%c%c]  RING: %d  HALTED: %d               ║\n",
           (cpu->flags & FLAG_V) ? 'V' : '-',
           (cpu->flags & FLAG_N) ? 'N' : '-',
           (cpu->flags & FLAG_C) ? 'C' : '-',
           (cpu->flags & FLAG_Z) ? 'Z' : '-',
           (cpu->flags & FLAG_P) ? 'P' : '-',
           (cpu->flags & FLAG_IP) ? 'I' : '-',
           cpu->ring, cpu->halted);
    printf("╟───────────────────────────────────────────────────────────╢\n");
    printf("║  Vector Registers (256-bit):                              ║\n");
    for (int i = 0; i < SUB64_NUM_VEC; i++) {
        printf("║  V%d: [%016lx %016lx %016lx %016lx] ║\n", i,
               (unsigned long)cpu->vec[i].lane[3],
               (unsigned long)cpu->vec[i].lane[2],
               (unsigned long)cpu->vec[i].lane[1],
               (unsigned long)cpu->vec[i].lane[0]);
    }
    printf("╚═══════════════════════════════════════════════════════════╝\n");
}
