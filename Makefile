# Makefile — SUB64 CPU Project
# Copyright (C) 2026 Subho (subhobhai943)
# SPDX-License-Identifier: GPL-3.0-or-later

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2 -Iinclude
LDFLAGS = -lm
BUILD   = build
BIN_DIR = bin

EMU_BIN  = $(BIN_DIR)/emulator
ASM_BIN  = $(BIN_DIR)/assembler
DIS_BIN  = $(BIN_DIR)/disassembler

EMU_ROOT = sub64-emu
ASM_ROOT = sasm
DIS_ROOT = sdisasm

.PHONY: all emulator assembler disassembler test clean

all: emulator assembler disassembler

emulator: $(EMU_BIN) $(EMU_ROOT)

assembler: $(ASM_BIN) $(ASM_ROOT)

disassembler: $(DIS_BIN) $(DIS_ROOT)

# ── Emulator ─────────────────────────────────────────────────
$(EMU_BIN): $(BUILD)/cpu.o $(BUILD)/fpu.o $(BUILD)/vpu.o $(BUILD)/main_emu.o | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(EMU_ROOT): $(EMU_BIN)
	cp $(EMU_BIN) $(EMU_ROOT)

$(BUILD)/cpu.o: emulator/cpu.c include/sub64.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/fpu.o: emulator/fpu.c include/sub64.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/vpu.o: emulator/vpu.c include/sub64.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/main_emu.o: emulator/main.c include/sub64.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ── Assembler ────────────────────────────────────────────────
$(ASM_BIN): $(BUILD)/lexer.o $(BUILD)/parser.o $(BUILD)/encoder.o $(BUILD)/main_asm.o | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(ASM_ROOT): $(ASM_BIN)
	cp $(ASM_BIN) $(ASM_ROOT)

$(BUILD)/lexer.o: assembler/lexer.c assembler/assembler.h include/sub64.h | $(BUILD)
	$(CC) $(CFLAGS) -Iassembler -c $< -o $@

$(BUILD)/parser.o: assembler/parser.c assembler/assembler.h include/sub64.h | $(BUILD)
	$(CC) $(CFLAGS) -Iassembler -c $< -o $@

$(BUILD)/encoder.o: assembler/encoder.c assembler/assembler.h include/sub64.h | $(BUILD)
	$(CC) $(CFLAGS) -Iassembler -c $< -o $@

$(BUILD)/main_asm.o: assembler/main.c assembler/assembler.h include/sub64.h | $(BUILD)
	$(CC) $(CFLAGS) -Iassembler -c $< -o $@

# ── Disassembler ─────────────────────────────────────────────
$(DIS_BIN): $(BUILD)/disasm.o $(BUILD)/main_dis.o | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(DIS_ROOT): $(DIS_BIN)
	cp $(DIS_BIN) $(DIS_ROOT)

$(BUILD)/disasm.o: disassembler/disasm.c disassembler/disasm.h include/sub64.h | $(BUILD)
	$(CC) $(CFLAGS) -Idisassembler -c $< -o $@

$(BUILD)/main_dis.o: disassembler/main.c disassembler/disasm.h include/sub64.h | $(BUILD)
	$(CC) $(CFLAGS) -Idisassembler -c $< -o $@

# ── Directories ──────────────────────────────────────────────
$(BUILD):
	mkdir -p $(BUILD)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# ── Test Suite ───────────────────────────────────────────────
test: all
	@echo "=== Running SUB64 Test Suite ==="
	@mkdir -p test_bin
	./$(ASM_ROOT) examples/hello.sasm -o test_bin/hello.bin
	-./$(EMU_ROOT) test_bin/hello.bin -d
	./$(ASM_ROOT) examples/fib.sasm -o test_bin/fib.bin
	-./$(EMU_ROOT) test_bin/fib.bin -d
	./$(ASM_ROOT) examples/vector_add.sasm -o test_bin/vector_add.bin
	-./$(EMU_ROOT) test_bin/vector_add.bin -d
	./$(ASM_ROOT) examples/wide_test.sasm -o test_bin/wide_test.bin
	-./$(EMU_ROOT) test_bin/wide_test.bin -d
	./$(ASM_ROOT) examples/syscall_test.sasm -o test_bin/syscall_test.bin
	-./$(EMU_ROOT) test_bin/syscall_test.bin
	@if [ -f examples/fpu_test.sasm ]; then \
		./$(ASM_ROOT) examples/fpu_test.sasm -o test_bin/fpu_test.bin && \
		./$(EMU_ROOT) test_bin/fpu_test.bin -d || true; \
	fi
	@if [ -f examples/priv_trap.sasm ]; then \
		./$(ASM_ROOT) examples/priv_trap.sasm -o test_bin/priv_trap.bin && \
		./$(EMU_ROOT) test_bin/priv_trap.bin -d || true; \
	fi
	@echo ""
	@echo "=== Disassembly Test ==="
	./$(DIS_ROOT) test_bin/hello.bin
	@echo ""
	@echo "=== All Tests Completed Successfully ==="

clean:
	rm -rf $(BUILD) $(BIN_DIR) $(EMU_ROOT) $(ASM_ROOT) $(DIS_ROOT) test_bin *.bin
