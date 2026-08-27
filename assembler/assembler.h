/*
 * assembler.h — Shared Types and Declarations for SASM
 * Copyright (C) 2026 Subho (subhobhai943)
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdint.h>
#include <stddef.h>
#include "sub64.h"

#define MAX_TOKENS       32
#define MAX_LABELS       512
#define MAX_INSTRUCTIONS 8192
#define MAX_LINE         512

typedef enum {
    TOK_MNEMONIC,
    TOK_REGISTER,
    TOK_VREG,
    TOK_SREG,
    TOK_IMMEDIATE,
    TOK_LABEL_DEF,
    TOK_LABEL_REF,
    TOK_COMMA,
    TOK_COLON,
    TOK_PIPE,
    TOK_EOF,
    TOK_UNKNOWN
} token_type;

typedef struct {
    token_type type;
    char       text[64];
    int64_t    value;
} token;

typedef struct {
    char     name[64];
    uint32_t address;
} label_entry;

typedef struct {
    token tokens[MAX_TOKENS];
    int   count;
} token_line;

typedef struct {
    label_entry labels[MAX_LABELS];
    int         label_count;
    uint8_t     output[MAX_INSTRUCTIONS * 16];
    uint32_t    output_size;
    int         error_count;
    char        error_msg[256];
} assembler_state;

/* Lexer */
int sasm_tokenize_line(const char *line, token_line *out);

/* Parser */
void sasm_init(assembler_state *as);
int  sasm_assemble_file(assembler_state *as, const char *filename);

/* Encoder */
uint32_t sasm_encode_instruction(const token_line *tl, const assembler_state *as, uint32_t current_addr);
int      sasm_encode_ldim(const token_line *tl, const assembler_state *as, uint8_t *out);

#endif /* ASSEMBLER_H */
