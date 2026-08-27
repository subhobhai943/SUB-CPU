/*
 * main.c — SUB64 Disassembler CLI
 * Copyright (C) 2026 Subho (subhobhai943)
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "disasm.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <binary_file>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize <= 0) {
        fprintf(stderr, "Error: empty binary file\n");
        fclose(f);
        return 1;
    }

    uint8_t *buf = (uint8_t *)malloc((size_t)fsize);
    if (!buf) {
        perror("malloc");
        fclose(f);
        return 1;
    }

    if (fread(buf, 1, (size_t)fsize, f) != (size_t)fsize) {
        perror("fread");
        free(buf);
        fclose(f);
        return 1;
    }
    fclose(f);

    printf("╔════════════════════════════════════════════════════════════════════╗\n");
    printf("║                 SUB64 Disassembly: %-31s ║\n", argv[1]);
    printf("╠══════════╦════════════════════════════════════╦════════════════════╣\n");
    printf("║ Address  ║ Raw Bytes                          ║ Instruction        ║\n");
    printf("╠══════════╬════════════════════════════════════╬════════════════════╣\n");

    uint32_t addr = 0;
    while (addr < (uint32_t)fsize) {
        char text[256];
        int consumed = sub64_disasm_instruction(buf + addr, (size_t)(fsize - addr),
                                                addr, text, sizeof(text));
        if (consumed <= 0) {
            printf("║ %08x ║ %02x                                   ║ .byte 0x%02x       ║\n",
                   addr, buf[addr], buf[addr]);
            addr++;
            continue;
        }

        char hex[64] = {0};
        int hpos = 0;
        for (int i = 0; i < consumed && i < 10; i++) {
            hpos += snprintf(hex + hpos, sizeof(hex) - hpos, "%02x ", buf[addr + i]);
        }

        printf("║ %08x ║ %-34s ║ %-18s ║\n", addr, hex, text);
        addr += consumed;
    }

    printf("╚══════════╩════════════════════════════════════╩════════════════════╝\n");
    printf("Total instructions size: %u bytes\n", (uint32_t)fsize);

    free(buf);
    return 0;
}
