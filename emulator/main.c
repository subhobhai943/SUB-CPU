/*
 * main.c — SUB-CPU Emulator Entry Point
 * Copyright (C) 2026 Subho (subhobhai943)
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include "../include/sub_cpu.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program.bin>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    sub_byte_t *prog = malloc(size);
    if (!prog) { fclose(f); return 1; }
    fread(prog, 1, size, f);
    fclose(f);

    sub_cpu_t cpu;
    sub_cpu_init(&cpu);
    sub_cpu_load(&cpu, prog, size, 0x0100);
    free(prog);

    printf("SUB-CPU Emulator starting...\n");
    sub_cpu_run(&cpu);
    sub_cpu_dump(&cpu);

    return 0;
}
