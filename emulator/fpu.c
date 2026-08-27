/*
 * fpu.c — Floating-Point Unit (64-bit IEEE-754)
 * Copyright (C) 2026 Subho (subhobhai943)
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "sub64.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static inline double u64_to_f64(uint64_t u) {
    double d;
    memcpy(&d, &u, sizeof(double));
    return d;
}

static inline uint64_t f64_to_u64(double d) {
    uint64_t u;
    memcpy(&u, &d, sizeof(uint64_t));
    return u;
}

void sub64_fpu_execute(sub64_cpu *cpu, uint32_t opcode, int rd, int ra, int rb) {
    uint64_t raw_a = (ra > 0 && ra < SUB64_NUM_GPR) ? cpu->gpr[ra] : 0;
    uint64_t raw_b = (rb > 0 && rb < SUB64_NUM_GPR) ? cpu->gpr[rb] : 0;

    double a = u64_to_f64(raw_a);
    double b = u64_to_f64(raw_b);
    double res = 0.0;
    int writes_rd = 1;

    switch (opcode) {
    case OP_FADD:
        res = a + b;
        break;

    case OP_FSUB:
        res = a - b;
        break;

    case OP_FMUL:
        res = a * b;
        break;

    case OP_FDIV:
        if (b == 0.0) {
            sub64_cpu_trap(cpu, VEC_ARITH_TRAP, 0);
            return;
        }
        res = a / b;
        break;

    case OP_FNEG:
        res = -a;
        break;

    case OP_FABS:
        res = fabs(a);
        break;

    case OP_FSQRT:
        if (a < 0.0) {
            sub64_cpu_trap(cpu, VEC_ARITH_TRAP, 0);
            return;
        }
        res = sqrt(a);
        break;

    case OP_FCMP:
        writes_rd = 0;
        cpu->flags &= ~(FLAG_Z | FLAG_N | FLAG_V | FLAG_C);
        if (isnan(a) || isnan(b)) {
            cpu->flags |= FLAG_V; /* Invalid comparison */
        } else if (a == b) {
            cpu->flags |= FLAG_Z;
        } else if (a < b) {
            cpu->flags |= FLAG_N;
        }
        break;

    case OP_FCVTIF:
        /* Integer in Ra -> Float in Rd */
        res = (double)((int64_t)raw_a);
        break;

    case OP_FCVTFI:
        /* Float in Ra -> Integer in Rd */
        writes_rd = 0;
        if (rd > 0 && rd < SUB64_NUM_GPR) {
            cpu->gpr[rd] = (uint64_t)((int64_t)a);
        }
        return;

    default:
        fprintf(stderr, "SUB64: Unknown FPU opcode 0x%02x\n", opcode);
        return;
    }

    if (writes_rd && rd > 0 && rd < SUB64_NUM_GPR) {
        cpu->gpr[rd] = f64_to_u64(res);
    }
}
