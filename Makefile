# Makefile — SUB64 CPU Project
# Copyright (C) 2026 Subho (subhobhai943)
# SPDX-License-Identifier: GPL-3.0-or-later

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2 -Iinclude
BUILD   = build

.PHONY: all emulator clean

all: emulator

emulator: $(BUILD)/cpu.o $(BUILD)/main_emu.o
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD)/cpu.o: emulator/cpu.c include/sub64.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/main_emu.o: emulator/main.c include/sub64.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD) emulator assembler *.bin
