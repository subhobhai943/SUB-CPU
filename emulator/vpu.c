/*
 * vpu.c — Vector processing unit (256-bit SIMD, 4 × 64-bit lanes)
 * Copyright (C) 2026 Subho (subhobhai943)
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "sub64.h"
#include <stdio.h>
#include <string.h>

static inline uint64_t vpu_mem_read64(sub64_cpu *cpu, sub64_word addr) {
    if (addr + 7 >= cpu->mem_size) { cpu->halted = 1; return 0; }
    uint64_t val = 0;
    for (int i = 0; i < 8; i++)
        val |= ((uint64_t)cpu->mem[addr + i]) << (i * 8);
    return val;
}

static inline void vpu_mem_write64(sub64_cpu *cpu, sub64_word addr, uint64_t val) {
    if (addr + 7 >= cpu->mem_size) { cpu->halted = 1; return; }
    for (int i = 0; i < 8; i++)
        cpu->mem[addr + i] = (uint8_t)((val >> (i * 8)) & 0xFF);
}

void sub64_vpu_execute(sub64_cpu *cpu, uint32_t opcode, int vd, int va, int vb, int rd, int ra) {
    /* Ensure vector registers are within bounds [0, SUB64_NUM_VEC) */
    vd &= (SUB64_NUM_VEC - 1);
    va &= (SUB64_NUM_VEC - 1);
    vb &= (SUB64_NUM_VEC - 1);

    switch (opcode) {
    case OP_VADD64:
        for (int i = 0; i < SUB64_VEC_LANES; i++)
            cpu->vec[vd].lane[i] = cpu->vec[va].lane[i] + cpu->vec[vb].lane[i];
        break;

    case OP_VSUB64:
        for (int i = 0; i < SUB64_VEC_LANES; i++)
            cpu->vec[vd].lane[i] = cpu->vec[va].lane[i] - cpu->vec[vb].lane[i];
        break;

    case OP_VMUL64:
        for (int i = 0; i < SUB64_VEC_LANES; i++)
            cpu->vec[vd].lane[i] = cpu->vec[va].lane[i] * cpu->vec[vb].lane[i];
        break;

    case OP_VDOT64: {
        uint64_t sum = 0;
        for (int i = 0; i < SUB64_VEC_LANES; i++)
            sum += cpu->vec[va].lane[i] * cpu->vec[vb].lane[i];
        if (rd > 0 && rd < SUB64_NUM_GPR)
            cpu->gpr[rd] = sum;
        break;
    }

    case OP_VLOAD: {
        sub64_word base = (ra >= 0 && ra < SUB64_NUM_GPR) ? cpu->gpr[ra] : 0;
        for (int i = 0; i < SUB64_VEC_LANES; i++)
            cpu->vec[vd].lane[i] = vpu_mem_read64(cpu, base + (uint64_t)(i * 8));
        break;
    }

    case OP_VSTORE: {
        /* VSTORE Ra, Vd: store Vd at MEM[cpu->gpr[ra]] */
        sub64_word base = (ra >= 0 && ra < SUB64_NUM_GPR) ? cpu->gpr[ra] : 0;
        for (int i = 0; i < SUB64_VEC_LANES; i++)
            vpu_mem_write64(cpu, base + (uint64_t)(i * 8), cpu->vec[vd].lane[i]);
        break;
    }

    case OP_VSHUFFLE: {
        sub64_vreg tmp;
        for (int i = 0; i < SUB64_VEC_LANES; i++) {
            int idx = (int)(cpu->vec[vb].lane[i] & (SUB64_VEC_LANES - 1));
            tmp.lane[i] = cpu->vec[va].lane[idx];
        }
        cpu->vec[vd] = tmp;
        break;
    }

    case OP_VBCAST: {
        uint64_t val = (ra >= 0 && ra < SUB64_NUM_GPR) ? cpu->gpr[ra] : 0;
        for (int i = 0; i < SUB64_VEC_LANES; i++)
            cpu->vec[vd].lane[i] = val;
        break;
    }

    default:
        fprintf(stderr, "SUB64: Unknown VPU opcode 0x%02x\n", opcode);
        break;
    }
}
