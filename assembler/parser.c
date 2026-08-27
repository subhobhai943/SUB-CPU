/*
 * parser.c — SASM Two-Pass Parser
 * Copyright (C) 2026 Subho (subhobhai943)
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include "assembler.h"

void sasm_init(assembler_state *as) {
    memset(as, 0, sizeof(assembler_state));
}

static void add_label(assembler_state *as, const char *name, uint32_t addr) {
    if (as->label_count >= MAX_LABELS) {
        fprintf(stderr, "SASM Error: label table overflow\n");
        as->error_count++;
        return;
    }
    snprintf(as->labels[as->label_count].name, sizeof(as->labels[as->label_count].name), "%s", name);
    as->labels[as->label_count].address = addr;
    as->label_count++;
}

static int line_instruction_size(const token_line *tl) {
    int has_wide = 0;
    int size = 0;

    for (int i = 0; i < tl->count; i++) {
        if (tl->tokens[i].type == TOK_MNEMONIC) {
            if (strcasecmp(tl->tokens[i].text, "WIDE") == 0) {
                has_wide = 1;
            } else if (strcasecmp(tl->tokens[i].text, "LDIM") == 0) {
                size = 10;
            } else {
                if (size == 0) size = 4;
            }
        }
    }
    if (has_wide) size = 8;
    return size;
}

int sasm_assemble_file(assembler_state *as, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        snprintf(as->error_msg, sizeof(as->error_msg), "Cannot open '%s'", filename);
        return -1;
    }

    char line[MAX_LINE];
    uint32_t current_addr = 0;

    /* ── Pass 1: Collect Labels ───────────────────────────── */
    while (fgets(line, sizeof(line), f)) {
        token_line tl;
        if (sasm_tokenize_line(line, &tl) > 0) {
            for (int i = 0; i < tl.count; i++) {
                if (tl.tokens[i].type == TOK_LABEL_DEF)
                    add_label(as, tl.tokens[i].text, current_addr);
            }
            current_addr += (uint32_t)line_instruction_size(&tl);
        }
    }

    /* ── Pass 2: Generate Code ────────────────────────────── */
    fseek(f, 0, SEEK_SET);
    current_addr = 0;

    while (fgets(line, sizeof(line), f)) {
        token_line tl;
        if (sasm_tokenize_line(line, &tl) <= 0) continue;

        /* Skip leading label definitions */
        int i = 0;
        while (i < tl.count && tl.tokens[i].type == TOK_LABEL_DEF) i++;
        if (i >= tl.count) continue;

        int is_wide = 0;
        if (tl.tokens[i].type == TOK_MNEMONIC && strcasecmp(tl.tokens[i].text, "WIDE") == 0) {
            is_wide = 1;
            i++;
        }

        token_line curr;
        curr.count = 0;
        for (; i < tl.count; i++)
            curr.tokens[curr.count++] = tl.tokens[i];

        if (curr.count == 0) continue;

        if (strcasecmp(curr.tokens[0].text, "LDIM") == 0) {
            int sz = sasm_encode_ldim(&curr, as, as->output + as->output_size);
            as->output_size += (uint32_t)sz;
            current_addr += (uint32_t)sz;
        } else if (is_wide) {
            /* Split at TOK_PIPE into slot_a and slot_b */
            token_line slot_a, slot_b;
            memset(&slot_a, 0, sizeof(slot_a));
            memset(&slot_b, 0, sizeof(slot_b));

            int p = 0;
            while (p < curr.count && curr.tokens[p].type != TOK_PIPE)
                slot_a.tokens[slot_a.count++] = curr.tokens[p++];
            if (p < curr.count && curr.tokens[p].type == TOK_PIPE) p++;
            while (p < curr.count)
                slot_b.tokens[slot_b.count++] = curr.tokens[p++];

            uint32_t w_a = sasm_encode_instruction(&slot_a, as, current_addr);
            uint32_t w_b = sasm_encode_instruction(&slot_b, as, current_addr);

            /*
             * Pack into 64-bit WIDE bundle:
             * [63]    1
             * [62:32] Slot A (31 bits)
             * [31]    1
             * [30:0]  Slot B (31 bits)
             */
            uint64_t wide_inst = (1ULL << 63) |
                                 (((uint64_t)w_a & 0x7FFFFFFF) << 32) |
                                 (1ULL << 31) |
                                 ((uint64_t)w_b & 0x7FFFFFFF);

            for (int b = 0; b < 8; b++)
                as->output[as->output_size + b] = (uint8_t)((wide_inst >> (b * 8)) & 0xFF);

            as->output_size += 8;
            current_addr += 8;
        } else {
            uint32_t word = sasm_encode_instruction(&curr, as, current_addr);
            as->output[as->output_size + 0] = (uint8_t)(word & 0xFF);
            as->output[as->output_size + 1] = (uint8_t)((word >> 8) & 0xFF);
            as->output[as->output_size + 2] = (uint8_t)((word >> 16) & 0xFF);
            as->output[as->output_size + 3] = (uint8_t)((word >> 24) & 0xFF);
            as->output_size += 4;
            current_addr += 4;
        }
    }

    fclose(f);
    return 0;
}
