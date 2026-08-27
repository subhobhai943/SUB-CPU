/*
 * main.c — SUB64 Emulator Entry Point
 * Copyright (C) 2026 Subho (subhobhai943)
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include "../include/sub64.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program.bin> [--dump]\n", argv[0]);
        fprintf(stderr, "  SUB64 Emulator — SUB Brand (C) 2026\n");
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror("fopen"); return 1; }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    sub64_byte *prog = malloc(size);
    if (!prog) { fclose(f); return 1; }
    fread(prog, 1, size, f);
    fclose(f);

    sub64_cpu cpu;
    if (sub64_cpu_init(&cpu, SUB64_MEM_SIZE) != 0) {
        fprintf(stderr, "Failed to allocate CPU memory\n");
        free(prog); return 1;
    }

    sub64_cpu_load(&cpu, prog, size, 0x1000);
    free(prog);

    printf("[SUB64] Starting emulator...\n");
    sub64_cpu_run(&cpu);

    int do_dump = (argc > 2 && argv[2][0] == '-');
    if (do_dump) sub64_cpu_dump(&cpu);

    sub64_cpu_free(&cpu);
    return 0;
}
