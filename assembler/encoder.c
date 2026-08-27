/*
 * encoder.c — SUB64 Binary Instruction Encoder
 * Copyright (C) 2026 Subho (subhobhai943)
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "assembler.h"

static uint32_t resolve_label(const assembler_state *as, const char *name) {
    for (int i = 0; i < as->label_count; i++) {
        if (strcmp(as->labels[i].name, name) == 0)
            return as->labels[i].address;
    }
    fprintf(stderr, "SASM Warning: unresolved label '%s'\n", name);
    return 0;
}

static int64_t get_operand_val(const token *t, const assembler_state *as,
                                uint32_t current_addr, int is_rel) {
    if (t->type == TOK_LABEL_REF) {
        uint32_t addr = resolve_label(as, t->text);
        if (is_rel)
            return ((int64_t)addr - (int64_t)current_addr) / 4;
        return (int64_t)addr;
    }
    return t->value;
}

int sasm_encode_ldim(const token_line *tl, const assembler_state *as, uint8_t *out) {
    uint32_t rd = 0;
    int64_t imm = 0;

    if (tl->count >= 2 && (tl->tokens[1].type == TOK_REGISTER || tl->tokens[1].type == TOK_VREG))
        rd = (uint32_t)tl->tokens[1].value;

    if (tl->count >= 3)
        imm = get_operand_val(&tl->tokens[2], as, 0, 0);

    /* 32-bit instruction word: [30:24]=OP_LDIM, [21:17]=rd */
    uint32_t instr = 0;
    instr |= ((uint32_t)OP_LDIM & 0x7F) << 24;
    instr |= (PRED_AL & 0x03) << 22;
    instr |= (rd & 0x1F) << 17;

    out[0] = (uint8_t)(instr & 0xFF);
    out[1] = (uint8_t)((instr >> 8) & 0xFF);
    out[2] = (uint8_t)((instr >> 16) & 0xFF);
    out[3] = (uint8_t)((instr >> 24) & 0xFF);

    /* 48-bit immediate (6 bytes little-endian) */
    for (int i = 0; i < 6; i++)
        out[4 + i] = (uint8_t)((imm >> (i * 8)) & 0xFF);

    return 10;
}

uint32_t sasm_encode_instruction(const token_line *tl, const assembler_state *as,
                                  uint32_t current_addr) {
    if (tl->count == 0) return 0;

    uint32_t op   = (uint32_t)tl->tokens[0].value;
    uint32_t pred = PRED_AL;
    uint32_t rd = 0, ra = 0, rb = 0, imm6 = 0;

    /*
     * Parse operands based on opcode category
     */

    /* Branches: BEQ Ra, Rb, label */
    if (op >= OP_BEQ && op <= OP_BGEU) {
        if (tl->count >= 4) {
            ra = (uint32_t)tl->tokens[1].value;
            rb = (uint32_t)tl->tokens[2].value;
            int64_t rel = get_operand_val(&tl->tokens[3], as, current_addr, 1);
            imm6 = (uint32_t)(rel & 0x3F);
        }
    }
    /* LOOP label: offset in rb (5 bits) and imm6 (6 bits) = 11-bit */
    else if (op == OP_LOOP) {
        if (tl->count >= 2) {
            int64_t rel = get_operand_val(&tl->tokens[1], as, current_addr, 1);
            uint32_t imm11 = (uint32_t)(rel & 0x7FF);
            rb   = (imm11 >> 6) & 0x1F;
            imm6 = imm11 & 0x3F;
        }
    }
    /* JMPI / CALLI label: 10-bit relative offset in ra + rb */
    else if (op == OP_JMPI || op == OP_CALLI) {
        if (tl->count >= 2) {
            int64_t rel = get_operand_val(&tl->tokens[1], as, current_addr, 1);
            uint32_t imm10 = (uint32_t)(rel & 0x3FF);
            ra = (imm10 >> 5) & 0x1F;
            rb = imm10 & 0x1F;
        }
    }
    /* JMP Ra, CALL Ra, PUSH Ra, POP Ra */
    else if (op == OP_JMP || op == OP_CALL || op == OP_PUSH || op == OP_POP) {
        if (tl->count >= 2)
            ra = (uint32_t)tl->tokens[1].value;
    }
    /* Stores: ST64 Ra, Rb [, imm6] */
    else if (op >= OP_ST64 && op <= OP_ST8) {
        if (tl->count >= 3) {
            ra = (uint32_t)tl->tokens[1].value;
            rb = (uint32_t)tl->tokens[2].value;
            if (tl->count >= 4) {
                int64_t off = get_operand_val(&tl->tokens[3], as, current_addr, 0);
                imm6 = (uint32_t)(off & 0x3F);
            }
        }
    }
    /* Loads: LD64 Rd, Ra [, imm6] */
    else if ((op >= OP_LD64 && op <= OP_LD8) || op == OP_LEA) {
        if (tl->count >= 3) {
            rd = (uint32_t)tl->tokens[1].value;
            ra = (uint32_t)tl->tokens[2].value;
            if (tl->count >= 4) {
                int64_t off = get_operand_val(&tl->tokens[3], as, current_addr, 0);
                imm6 = (uint32_t)(off & 0x3F);
            }
        }
    }
    /* CMP / TST / FCMP Ra, Rb */
    else if (op == OP_CMP || op == OP_TST || op == OP_FCMP) {
        if (tl->count >= 3) {
            ra = (uint32_t)tl->tokens[1].value;
            rb = (uint32_t)tl->tokens[2].value;
        }
    }
    /* CMPI / TSTI Ra, imm6 */
    else if (op == OP_CMPI || op == OP_TSTI) {
        if (tl->count >= 3) {
            ra = (uint32_t)tl->tokens[1].value;
            int64_t val = get_operand_val(&tl->tokens[2], as, current_addr, 0);
            imm6 = (uint32_t)(val & 0x3F);
        }
    }
    /* Vector Store: VSTORE Ra, Vd (or VSTORE Vd, Ra) */
    else if (op == OP_VSTORE) {
        if (tl->count >= 3) {
            if (tl->tokens[1].type == TOK_REGISTER) {
                ra = (uint32_t)tl->tokens[1].value;
                rd = (uint32_t)tl->tokens[2].value;
            } else {
                rd = (uint32_t)tl->tokens[1].value;
                ra = (uint32_t)tl->tokens[2].value;
            }
        }
    }
    /* Vector Load / Broadcast: VLOAD Vd, Ra / VBROADCAST Vd, Ra */
    else if (op == OP_VLOAD || op == OP_VBCAST) {
        if (tl->count >= 3) {
            rd = (uint32_t)tl->tokens[1].value;
            ra = (uint32_t)tl->tokens[2].value;
        }
    }
    /* Vector 3-op: VADD.64 Vd, Va, Vb / VSUB.64 / VMUL.64 / VDOT.64 / VSHUFFLE */
    else if (op >= OP_VADD64 && op <= OP_VSHUFFLE) {
        if (tl->count >= 4) {
            rd = (uint32_t)tl->tokens[1].value;
            ra = (uint32_t)tl->tokens[2].value;
            rb = (uint32_t)tl->tokens[3].value;
        }
    }
    /* System: RSR Rd, SREG */
    else if (op == OP_RSR) {
        if (tl->count >= 3) {
            rd = (uint32_t)tl->tokens[1].value;
            ra = (uint32_t)tl->tokens[2].value;
        }
    }
    /* System: WSR SREG, Ra */
    else if (op == OP_WSR) {
        if (tl->count >= 3) {
            rd = (uint32_t)tl->tokens[1].value; /* sreg index */
            ra = (uint32_t)tl->tokens[2].value; /* source gpr */
        }
    }
    /* System: INT imm6 */
    else if (op == OP_INT) {
        if (tl->count >= 2) {
            int64_t val = get_operand_val(&tl->tokens[1], as, current_addr, 0);
            imm6 = (uint32_t)(val & 0x3F);
        }
    }
    /* No-operand instructions */
    else if (op == OP_RET || op == OP_HLT || op == OP_FENCE ||
             op == OP_SYSCALL || op == OP_SYSRET) {
        rd = ra = rb = imm6 = 0;
    }
    /* Default 3-op or 2-op instructions (ADD, SUB, FADD, FNEG, MOV, etc.) */
    else {
        int t = 1;
        if (t < tl->count && (tl->tokens[t].type == TOK_REGISTER ||
                              tl->tokens[t].type == TOK_VREG ||
                              tl->tokens[t].type == TOK_SREG))
            rd = (uint32_t)tl->tokens[t++].value;
        if (t < tl->count && (tl->tokens[t].type == TOK_REGISTER ||
                              tl->tokens[t].type == TOK_VREG ||
                              tl->tokens[t].type == TOK_SREG))
            ra = (uint32_t)tl->tokens[t++].value;
        if (t < tl->count) {
            if (tl->tokens[t].type == TOK_REGISTER ||
                tl->tokens[t].type == TOK_VREG ||
                tl->tokens[t].type == TOK_SREG) {
                rb = (uint32_t)tl->tokens[t].value;
            } else {
                int64_t val = get_operand_val(&tl->tokens[t], as, current_addr, 0);
                imm6 = (uint32_t)(val & 0x3F);
            }
        }
    }

    /*
     * Pack into 32-bit instruction word:
     * [31]    0 (standard instruction)
     * [30:24] OPCODE (7 bits)
     * [23:22] PRED   (2 bits)
     * [21:17] RD     (5 bits)
     * [16:12] RA     (5 bits)
     * [11:7]  RB     (5 bits)
     * [6:0]   IMM6 in [5:0]
     */
    uint32_t inst = 0;
    inst |= (op   & 0x7F) << 24;
    inst |= (pred & 0x03) << 22;
    inst |= (rd   & 0x1F) << 17;
    inst |= (ra   & 0x1F) << 12;
    inst |= (rb   & 0x1F) <<  7;
    inst |= (imm6 & 0x3F);

    return inst;
}
