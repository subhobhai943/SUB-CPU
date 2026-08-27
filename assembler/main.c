/*
 * main.c — SASM Assembler CLI Driver
 * Copyright (C) 2026 Subho (subhobhai943)
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <string.h>
#include "assembler.h"

static void usage(const char *prog) {
    fprintf(stderr,
        "SUB64 Assembler (SASM) v1.0\n"
        "Usage: %s <input.sasm> [-o <output.bin>]\n", prog);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    const char *infile  = argv[1];
    const char *outfile = "out.bin";

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            outfile = argv[++i];
        }
    }

    assembler_state as;
    sasm_init(&as);

    if (sasm_assemble_file(&as, infile) != 0) {
        fprintf(stderr, "Error: %s\n", as.error_msg);
        return 1;
    }

    if (as.error_count > 0) {
        fprintf(stderr, "Assembly failed with %d error(s)\n", as.error_count);
        return 1;
    }

    FILE *out = fopen(outfile, "wb");
    if (!out) {
        perror("fopen");
        return 1;
    }

    size_t written = fwrite(as.output, 1, as.output_size, out);
    fclose(out);

    if (written != as.output_size) {
        fprintf(stderr, "Error writing output file\n");
        return 1;
    }

    printf("✓ Assembled '%s' → '%s' (%u bytes, %d labels)\n",
           infile, outfile, as.output_size, as.label_count);

    return 0;
}
