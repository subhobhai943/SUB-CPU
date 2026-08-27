/*
 * disasm.c — SUB64 Instruction Disassembler
 * Copyright (C) 2026 Subho (subhobhai943)
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "disasm.h"
#include <stdio.h>
#include <string.h>

static const char *sreg_names[] = {
    "flags", "kvec", "uvec", "ring", "lr", "pc"
};

static const char *pred_suffixes[] = {
    "", ".eq", ".ne", ".cs"
};

static void format_single_op(uint32_t inst, uint32_t addr, char *out, size_t out_len) {
    uint32_t op   = INSTR_OPCODE(inst);
    uint32_t pred = INSTR_PRED(inst);
    uint32_t rd   = INSTR_RD(inst);
    uint32_t ra   = INSTR_RA(inst);
    uint32_t rb   = INSTR_RB(inst);
    uint32_t imm6 = INSTR_IMM6(inst);
    int64_t simm6 = (int64_t)((int32_t)(imm6 << 26) >> 26);

    const char *psuf = pred_suffixes[pred & 3];

    switch (op) {
    case OP_ADD:  snprintf(out, out_len, "ADD%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_ADDI: snprintf(out, out_len, "ADDI%s  X%u, X%u, %ld", psuf, rd, ra, (long)simm6); break;
    case OP_SUB:  snprintf(out, out_len, "SUB%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_SUBI: snprintf(out, out_len, "SUBI%s  X%u, X%u, %ld", psuf, rd, ra, (long)simm6); break;
    case OP_MUL:  snprintf(out, out_len, "MUL%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_MULH: snprintf(out, out_len, "MULH%s  X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_DIV:  snprintf(out, out_len, "DIV%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_REM:  snprintf(out, out_len, "REM%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_NEG:  snprintf(out, out_len, "NEG%s   X%u, X%u", psuf, rd, ra); break;
    case OP_ABS:  snprintf(out, out_len, "ABS%s   X%u, X%u", psuf, rd, ra); break;

    case OP_AND:  snprintf(out, out_len, "AND%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_ANDI: snprintf(out, out_len, "ANDI%s  X%u, X%u, 0x%x", psuf, rd, ra, imm6); break;
    case OP_OR:   snprintf(out, out_len, "OR%s    X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_ORI:  snprintf(out, out_len, "ORI%s   X%u, X%u, 0x%x", psuf, rd, ra, imm6); break;
    case OP_XOR:  snprintf(out, out_len, "XOR%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_NOT:  snprintf(out, out_len, "NOT%s   X%u, X%u", psuf, rd, ra); break;
    case OP_SHL:  snprintf(out, out_len, "SHL%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_SHR:  snprintf(out, out_len, "SHR%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_SAR:  snprintf(out, out_len, "SAR%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_ROL:  snprintf(out, out_len, "ROL%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_ROR:  snprintf(out, out_len, "ROR%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_CLZ:  snprintf(out, out_len, "CLZ%s   X%u, X%u", psuf, rd, ra); break;
    case OP_CTZ:  snprintf(out, out_len, "CTZ%s   X%u, X%u", psuf, rd, ra); break;
    case OP_POPC: snprintf(out, out_len, "POPC%s  X%u, X%u", psuf, rd, ra); break;
    case OP_BEXT: snprintf(out, out_len, "BEXT%s  X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_BDEP: snprintf(out, out_len, "BDEP%s  X%u, X%u, X%u", psuf, rd, ra, rb); break;

    case OP_LD64: snprintf(out, out_len, "LD64%s  X%u, X%u, %ld", psuf, rd, ra, (long)simm6); break;
    case OP_LD32: snprintf(out, out_len, "LD32%s  X%u, X%u, %ld", psuf, rd, ra, (long)simm6); break;
    case OP_LD16: snprintf(out, out_len, "LD16%s  X%u, X%u, %ld", psuf, rd, ra, (long)simm6); break;
    case OP_LD8:  snprintf(out, out_len, "LD8%s   X%u, X%u, %ld", psuf, rd, ra, (long)simm6); break;
    case OP_ST64: snprintf(out, out_len, "ST64%s  X%u, X%u, %ld", psuf, ra, rb, (long)simm6); break;
    case OP_ST32: snprintf(out, out_len, "ST32%s  X%u, X%u, %ld", psuf, ra, rb, (long)simm6); break;
    case OP_ST16: snprintf(out, out_len, "ST16%s  X%u, X%u, %ld", psuf, ra, rb, (long)simm6); break;
    case OP_ST8:  snprintf(out, out_len, "ST8%s   X%u, X%u, %ld", psuf, ra, rb, (long)simm6); break;
    case OP_LEA:  snprintf(out, out_len, "LEA%s   X%u, X%u, %ld", psuf, rd, ra, (long)simm6); break;
    case OP_PUSH: snprintf(out, out_len, "PUSH%s  X%u", psuf, ra); break;
    case OP_POP:  snprintf(out, out_len, "POP%s   X%u", psuf, ra); break;
    case OP_XCHG: snprintf(out, out_len, "XCHG%s  X%u, X%u", psuf, rd, ra); break;
    case OP_CAS:  snprintf(out, out_len, "CAS%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;

    case OP_JMP:  snprintf(out, out_len, "JMP%s   X%u", psuf, ra); break;
    case OP_JMPI: {
        int32_t rel = (int32_t)((ra << 5) | rb);
        rel = (rel << 22) >> 22;
        snprintf(out, out_len, "JMPI%s  0x%x", psuf, (uint32_t)(addr + (rel << 2)));
        break;
    }
    case OP_CALL: snprintf(out, out_len, "CALL%s  X%u", psuf, ra); break;
    case OP_CALLI: {
        int32_t rel = (int32_t)((ra << 5) | rb);
        rel = (rel << 22) >> 22;
        snprintf(out, out_len, "CALLI%s 0x%x", psuf, (uint32_t)(addr + (rel << 2)));
        break;
    }
    case OP_RET:  snprintf(out, out_len, "RET%s", psuf); break;
    case OP_BEQ:  snprintf(out, out_len, "BEQ%s   X%u, X%u, 0x%x", psuf, ra, rb, (uint32_t)(addr + (simm6 << 2))); break;
    case OP_BNE:  snprintf(out, out_len, "BNE%s   X%u, X%u, 0x%x", psuf, ra, rb, (uint32_t)(addr + (simm6 << 2))); break;
    case OP_BLT:  snprintf(out, out_len, "BLT%s   X%u, X%u, 0x%x", psuf, ra, rb, (uint32_t)(addr + (simm6 << 2))); break;
    case OP_BGE:  snprintf(out, out_len, "BGE%s   X%u, X%u, 0x%x", psuf, ra, rb, (uint32_t)(addr + (simm6 << 2))); break;
    case OP_BLTU: snprintf(out, out_len, "BLTU%s  X%u, X%u, 0x%x", psuf, ra, rb, (uint32_t)(addr + (simm6 << 2))); break;
    case OP_BGEU: snprintf(out, out_len, "BGEU%s  X%u, X%u, 0x%x", psuf, ra, rb, (uint32_t)(addr + (simm6 << 2))); break;
    case OP_LOOP: {
        int32_t off11 = (int32_t)((rb << 6) | imm6);
        off11 = (off11 << 21) >> 21;
        snprintf(out, out_len, "LOOP%s  0x%x", psuf, (uint32_t)(addr + (off11 << 2)));
        break;
    }
    case OP_SYSCALL: snprintf(out, out_len, "SYSCALL%s", psuf); break;
    case OP_SYSRET:  snprintf(out, out_len, "SYSRET%s", psuf); break;
    case OP_FENCE:   snprintf(out, out_len, "FENCE%s", psuf); break;
    case OP_HLT:     snprintf(out, out_len, "HLT%s", psuf); break;

    case OP_CMP:  snprintf(out, out_len, "CMP%s   X%u, X%u", psuf, ra, rb); break;
    case OP_CMPI: snprintf(out, out_len, "CMPI%s  X%u, %ld", psuf, ra, (long)simm6); break;
    case OP_TST:  snprintf(out, out_len, "TST%s   X%u, X%u", psuf, ra, rb); break;
    case OP_TSTI: snprintf(out, out_len, "TSTI%s  X%u, %ld", psuf, ra, (long)simm6); break;
    case OP_SEQ:  snprintf(out, out_len, "SEQ%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_SNE:  snprintf(out, out_len, "SNE%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_SLT:  snprintf(out, out_len, "SLT%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_SLTU: snprintf(out, out_len, "SLTU%s  X%u, X%u, X%u", psuf, rd, ra, rb); break;

    case OP_MOV:   snprintf(out, out_len, "MOV%s   X%u, X%u", psuf, rd, ra); break;
    case OP_MOVZ:  snprintf(out, out_len, "MOVZ%s  X%u, X%u", psuf, rd, ra); break;
    case OP_MOVNZ: snprintf(out, out_len, "MOVNZ%s X%u, X%u", psuf, rd, ra); break;
    case OP_MOVC:  snprintf(out, out_len, "MOVC%s  X%u, X%u", psuf, rd, ra); break;
    case OP_MOVN:  snprintf(out, out_len, "MOVN%s  X%u, X%u", psuf, rd, ra); break;
    case OP_SEL:   snprintf(out, out_len, "SEL%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;

    case OP_ZEXT32: snprintf(out, out_len, "ZEXT32%s X%u, X%u", psuf, rd, ra); break;
    case OP_SEXT32: snprintf(out, out_len, "SEXT32%s X%u, X%u", psuf, rd, ra); break;
    case OP_SEXT16: snprintf(out, out_len, "SEXT16%s X%u, X%u", psuf, rd, ra); break;
    case OP_SEXT8:  snprintf(out, out_len, "SEXT8%s  X%u, X%u", psuf, rd, ra); break;

    /* Vector */
    case OP_VADD64:   snprintf(out, out_len, "VADD.64  V%u, V%u, V%u", rd, ra, rb); break;
    case OP_VSUB64:   snprintf(out, out_len, "VSUB.64  V%u, V%u, V%u", rd, ra, rb); break;
    case OP_VMUL64:   snprintf(out, out_len, "VMUL.64  V%u, V%u, V%u", rd, ra, rb); break;
    case OP_VDOT64:   snprintf(out, out_len, "VDOT.64  X%u, V%u, V%u", rd, ra, rb); break;
    case OP_VLOAD:    snprintf(out, out_len, "VLOAD    V%u, X%u", rd, ra); break;
    case OP_VSTORE:   snprintf(out, out_len, "VSTORE   X%u, V%u", ra, rd); break;
    case OP_VSHUFFLE: snprintf(out, out_len, "VSHUFFLE V%u, V%u, V%u", rd, ra, rb); break;
    case OP_VBCAST:   snprintf(out, out_len, "VBROADCAST V%u, X%u", rd, ra); break;

    /* FPU */
    case OP_FADD:   snprintf(out, out_len, "FADD%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_FSUB:   snprintf(out, out_len, "FSUB%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_FMUL:   snprintf(out, out_len, "FMUL%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_FDIV:   snprintf(out, out_len, "FDIV%s   X%u, X%u, X%u", psuf, rd, ra, rb); break;
    case OP_FNEG:   snprintf(out, out_len, "FNEG%s   X%u, X%u", psuf, rd, ra); break;
    case OP_FABS:   snprintf(out, out_len, "FABS%s   X%u, X%u", psuf, rd, ra); break;
    case OP_FSQRT:  snprintf(out, out_len, "FSQRT%s  X%u, X%u", psuf, rd, ra); break;
    case OP_FCMP:   snprintf(out, out_len, "FCMP%s   X%u, X%u", psuf, ra, rb); break;
    case OP_FCVTIF: snprintf(out, out_len, "FCVTIF%s X%u, X%u", psuf, rd, ra); break;
    case OP_FCVTFI: snprintf(out, out_len, "FCVTFI%s X%u, X%u", psuf, rd, ra); break;

    /* System */
    case OP_RSR: {
        const char *sr = (ra < 6) ? sreg_names[ra] : "unknown";
        snprintf(out, out_len, "RSR%s    X%u, %s", psuf, rd, sr);
        break;
    }
    case OP_WSR: {
        const char *sr = (rd < 6) ? sreg_names[rd] : "unknown";
        snprintf(out, out_len, "WSR%s    %s, X%u", psuf, sr, ra);
        break;
    }
    case OP_INT:
        snprintf(out, out_len, "INT%s    %u", psuf, imm6);
        break;

    default:
        snprintf(out, out_len, ".word 0x%08x ; unknown opcode 0x%02x", inst, op);
        break;
    }
}

int sub64_disasm_instruction(const uint8_t *buffer, size_t buf_len,
                             uint32_t addr, char *out, size_t out_len) {
    if (buf_len < 4) return 0;

    uint32_t inst = (uint32_t)buffer[0] |
                    ((uint32_t)buffer[1] << 8) |
                    ((uint32_t)buffer[2] << 16) |
                    ((uint32_t)buffer[3] << 24);

    if (INSTR_IS_WIDE(inst)) {
        if (buf_len < 8) return 0;
        uint64_t inst64 = 0;
        for (int i = 0; i < 8; i++)
            inst64 |= ((uint64_t)buffer[i]) << (i * 8);

        uint32_t slot_a = (uint32_t)((inst64 >> 32) & 0x7FFFFFFF);
        uint32_t slot_b = (uint32_t)(inst64 & 0x7FFFFFFF);

        char text_a[128], text_b[128];
        format_single_op(slot_a, addr, text_a, sizeof(text_a));
        format_single_op(slot_b, addr, text_b, sizeof(text_b));

        snprintf(out, out_len, "WIDE  %s | %s", text_a, text_b);
        return 8;
    }

    uint32_t op = INSTR_OPCODE(inst);
    if (op == OP_LDIM) {
        if (buf_len < 10) return 0;
        uint32_t rd = INSTR_RD(inst);
        uint64_t lo = (uint32_t)buffer[4] | ((uint32_t)buffer[5] << 8) |
                      ((uint32_t)buffer[6] << 16) | ((uint32_t)buffer[7] << 24);
        uint64_t hi = (uint32_t)buffer[8] | ((uint32_t)buffer[9] << 8);
        uint64_t imm48 = lo | (hi << 32);

        snprintf(out, out_len, "LDIM    X%u, 0x%lx (%lu)", rd,
                 (unsigned long)imm48, (unsigned long)imm48);
        return 10;
    }

    format_single_op(inst, addr, out, out_len);
    return 4;
}
