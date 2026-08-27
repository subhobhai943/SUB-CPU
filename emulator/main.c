/*
 * main.c — SUB64 Emulator CLI Driver
 * Copyright (C) 2026 Subho (subhobhai943)
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "sub64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *prog) {
    fprintf(stderr,
        "SUB64 Emulator v1.0\n"
        "Usage: %s <binary_file> [-d]\n"
        "  -d    Dump register state after execution\n", prog);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    int dump = 0;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) dump = 1;
    }

    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize <= 0) {
        fprintf(stderr, "Error: empty or invalid file '%s'\n", filename);
        fclose(f);
        return 1;
    }

    sub64_byte *prog = (sub64_byte *)malloc((size_t)fsize);
    if (!prog) {
        perror("malloc");
        fclose(f);
        return 1;
    }

    if (fread(prog, 1, (size_t)fsize, f) != (size_t)fsize) {
        perror("fread");
        free(prog);
        fclose(f);
        return 1;
    }
    fclose(f);

    sub64_cpu cpu;
    if (sub64_cpu_init(&cpu, SUB64_MEM_SIZE) != 0) {
        fprintf(stderr, "Error: failed to allocate CPU memory\n");
        free(prog);
        return 1;
    }

    sub64_cpu_load(&cpu, prog, (size_t)fsize, 0);
    free(prog);

    sub64_cpu_run(&cpu);

    if (dump) {
        sub64_cpu_dump(&cpu);
    }

    int exit_code = (int)(cpu.gpr[REG_A0] & 0xFF);
    sub64_cpu_free(&cpu);
    return exit_code;
}
