/*
 * disasm.h — SUB64 Disassembler Library
 * Copyright (C) 2026 Subho (subhobhai943)
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DISASM_H
#define DISASM_H

#include <stdint.h>
#include <stddef.h>
#include "sub64.h"

/* Disassembles an instruction at 'buffer' (at logical 'addr').
 * Returns the number of bytes consumed (4, 8, or 10), or 0 on error.
 * Writes formatted text into 'out' (at least 128 bytes).
 */
int sub64_disasm_instruction(const uint8_t *buffer, size_t buf_len,
                             uint32_t addr, char *out, size_t out_len);

#endif /* DISASM_H */
