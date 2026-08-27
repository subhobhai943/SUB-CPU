/*
 * lexer.c — SASM Tokenizer
 * Copyright (C) 2026 Subho (subhobhai943)
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <strings.h>
#include "assembler.h"

static int is_special_register(const char *text, int64_t *val, token_type *t_type) {
    static const struct { const char *name; int sreg; } sregs[] = {
        {"flags", SREG_FLAGS},
        {"kvec",  SREG_KVEC},
        {"uvec",  SREG_UVEC},
        {"ring",  SREG_RING},
        {"lr",    SREG_LR},
        {"pc",    SREG_PC},
        {NULL, 0}
    };
    for (int i = 0; sregs[i].name; i++) {
        if (strcasecmp(text, sregs[i].name) == 0) {
            *val = sregs[i].sreg;
            *t_type = TOK_SREG;
            return 1;
        }
    }
    return 0;
}

static int is_register(const char *text, int64_t *val, token_type *t_type) {
    /* GPRs: X0–X23 */
    if ((text[0] == 'X' || text[0] == 'x') && isdigit((unsigned char)text[1])) {
        char *end;
        long r = strtol(text + 1, &end, 10);
        if (*end == '\0' && r >= 0 && r < SUB64_NUM_GPR) {
            *val = r;
            *t_type = TOK_REGISTER;
            return 1;
        }
    }

    /* Vector registers: V0–V7 */
    if ((text[0] == 'V' || text[0] == 'v') && isdigit((unsigned char)text[1])) {
        char *end;
        long r = strtol(text + 1, &end, 10);
        if (*end == '\0' && r >= 0 && r < SUB64_NUM_VEC) {
            *val = r;
            *t_type = TOK_VREG;
            return 1;
        }
    }

    /* ABI Register Aliases */
    static const struct { const char *name; int reg; } aliases[] = {
        {"zero", REG_ZERO}, {"ra", REG_RA}, {"sp", REG_SP}, {"fp", REG_FP},
        {"a0", REG_A0},     {"a1", REG_A1}, {"a2", REG_A2}, {"a3", REG_A3},
        {"t0", REG_T0},     {"t1", REG_T0+1}, {"t2", REG_T0+2}, {"t3", REG_T0+3},
        {"t4", REG_T0+4},   {"t5", REG_T0+5}, {"t6", REG_T0+6}, {"t7", REG_T0+7},
        {"s0", REG_S0},     {"s1", REG_S0+1}, {"s2", REG_S0+2}, {"s3", REG_S0+3},
        {"s4", REG_S0+4},   {"s5", REG_S0+5}, {"s6", REG_S0+6}, {"s7", REG_S0+7},
        {NULL, 0}
    };

    for (int i = 0; aliases[i].name; i++) {
        if (strcasecmp(text, aliases[i].name) == 0) {
            *val = aliases[i].reg;
            *t_type = TOK_REGISTER;
            return 1;
        }
    }
    return 0;
}

static int is_mnemonic(const char *text, int64_t *val) {
    static const struct { const char *name; int op; } ops[] = {
        /* Arithmetic */
        {"ADD", OP_ADD},     {"ADDI", OP_ADDI},   {"SUB", OP_SUB},     {"SUBI", OP_SUBI},
        {"MUL", OP_MUL},     {"MULH", OP_MULH},   {"DIV", OP_DIV},     {"REM", OP_REM},
        {"NEG", OP_NEG},     {"ABS", OP_ABS},

        /* Logic */
        {"AND", OP_AND},     {"ANDI", OP_ANDI},   {"OR", OP_OR},       {"ORI", OP_ORI},
        {"XOR", OP_XOR},     {"NOT", OP_NOT},     {"SHL", OP_SHL},     {"SHR", OP_SHR},
        {"SAR", OP_SAR},     {"ROL", OP_ROL},     {"ROR", OP_ROR},     {"CLZ", OP_CLZ},
        {"CTZ", OP_CTZ},     {"POPC", OP_POPC},   {"BEXT", OP_BEXT},   {"BDEP", OP_BDEP},

        /* Memory */
        {"LD64", OP_LD64},   {"LD32", OP_LD32},   {"LD16", OP_LD16},   {"LD8", OP_LD8},
        {"ST64", OP_ST64},   {"ST32", OP_ST32},   {"ST16", OP_ST16},   {"ST8", OP_ST8},
        {"LDIM", OP_LDIM},   {"LEA", OP_LEA},     {"PUSH", OP_PUSH},   {"POP", OP_POP},
        {"XCHG", OP_XCHG},   {"CAS", OP_CAS},

        /* Control flow */
        {"JMP", OP_JMP},     {"JMPI", OP_JMPI},   {"CALL", OP_CALL},   {"CALLI", OP_CALLI},
        {"RET", OP_RET},     {"BEQ", OP_BEQ},     {"BNE", OP_BNE},     {"BLT", OP_BLT},
        {"BGE", OP_BGE},     {"BLTU", OP_BLTU},   {"BGEU", OP_BGEU},   {"LOOP", OP_LOOP},
        {"SYSCALL", OP_SYSCALL}, {"SYSRET", OP_SYSRET}, {"FENCE", OP_FENCE}, {"HLT", OP_HLT},

        /* Compare & Select */
        {"CMP", OP_CMP},     {"CMPI", OP_CMPI},   {"TST", OP_TST},     {"TSTI", OP_TSTI},
        {"SEQ", OP_SEQ},     {"SNE", OP_SNE},     {"SLT", OP_SLT},     {"SLTU", OP_SLTU},

        /* Move */
        {"MOV", OP_MOV},     {"MOVZ", OP_MOVZ},   {"MOVNZ", OP_MOVNZ}, {"MOVC", OP_MOVC},
        {"MOVN", OP_MOVN},   {"SEL", OP_SEL},     {"ZEXT32", OP_ZEXT32},{"SEXT32", OP_SEXT32},
        {"SEXT16", OP_SEXT16}, {"SEXT8", OP_SEXT8},

        /* Vector / SIMD */
        {"VADD.64", OP_VADD64}, {"VADD64", OP_VADD64},
        {"VSUB.64", OP_VSUB64}, {"VSUB64", OP_VSUB64},
        {"VMUL.64", OP_VMUL64}, {"VMUL64", OP_VMUL64},
        {"VDOT.64", OP_VDOT64}, {"VDOT64", OP_VDOT64},
        {"VLOAD", OP_VLOAD},
        {"VSTORE", OP_VSTORE},
        {"VSHUFFLE", OP_VSHUFFLE},
        {"VBROADCAST", OP_VBCAST}, {"VBCAST", OP_VBCAST},

        /* Floating-Point Unit */
        {"FADD", OP_FADD},   {"FSUB", OP_FSUB},   {"FMUL", OP_FMUL},   {"FDIV", OP_FDIV},
        {"FNEG", OP_FNEG},   {"FABS", OP_FABS},   {"FSQRT", OP_FSQRT}, {"FCMP", OP_FCMP},
        {"FCVTIF", OP_FCVTIF}, {"FCVTFI", OP_FCVTFI},

        /* System & Special Registers */
        {"RSR", OP_RSR},     {"WSR", OP_WSR},     {"INT", OP_INT},

        /* Pseudo */
        {"WIDE", -1},

        {NULL, 0}
    };

    for (int i = 0; ops[i].name; i++) {
        if (strcasecmp(text, ops[i].name) == 0) {
            *val = ops[i].op;
            return 1;
        }
    }
    return 0;
}

int sasm_tokenize_line(const char *line, token_line *out) {
    out->count = 0;
    const char *p = line;
    int expect_mnemonic = 1;

    while (*p) {
        while (isspace((unsigned char)*p)) p++;
        if (!*p) break;

        /* Comment starts with ';' or '#' */
        if (*p == ';' || *p == '#') break;

        /* Comma is an optional delimiter */
        if (*p == ',') {
            p++;
            continue;
        }

        /* Pipe for WIDE dual issue */
        if (*p == '|') {
            if (out->count < MAX_TOKENS) {
                token *t = &out->tokens[out->count++];
                t->type = TOK_PIPE;
                snprintf(t->text, sizeof(t->text), "|");
                t->value = 0;
            }
            expect_mnemonic = 1; /* slot B begins */
            p++;
            continue;
        }

        /* Token characters */
        char buf[64];
        int i = 0;
        while (*p && !isspace((unsigned char)*p) && *p != ',' && *p != ';' && *p != '#' && *p != '|') {
            if (i < 63) buf[i++] = *p;
            p++;
        }
        buf[i] = '\0';
        if (i == 0) continue;

        if (out->count >= MAX_TOKENS) break;
        token *t = &out->tokens[out->count++];
        snprintf(t->text, sizeof(t->text), "%s", buf);

        int len = (int)strlen(buf);
        if (len > 1 && buf[len - 1] == ':') {
            /* Label definition */
            t->type = TOK_LABEL_DEF;
            t->text[len - 1] = '\0';
            t->value = 0;
            expect_mnemonic = 1; /* instruction can follow label on same line */
        } else if (expect_mnemonic && is_mnemonic(buf, &t->value)) {
            t->type = TOK_MNEMONIC;
            if (strcasecmp(buf, "WIDE") == 0) {
                expect_mnemonic = 1; /* WIDE is followed by slot A mnemonic */
            } else {
                expect_mnemonic = 0; /* next tokens are operands */
            }
        } else if (is_register(buf, &t->value, &t->type)) {
            /* Handled in is_register */
        } else if (is_special_register(buf, &t->value, &t->type)) {
            /* Handled in is_special_register */
        } else {
            char *endptr;
            int64_t val = strtoll(buf, &endptr, 0);
            if (*endptr == '\0') {
                t->type = TOK_IMMEDIATE;
                t->value = val;
            } else {
                t->type = TOK_LABEL_REF;
                t->value = 0;
            }
        }
    }
    return out->count;
}
