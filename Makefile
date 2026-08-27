# Makefile for SUB-CPU Project
# Copyright (C) 2026 Subho (subhobhai943)
# SPDX-License-Identifier: GPL-3.0-or-later

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -Iinclude
OBJDIR  = build

.PHONY: all emulator clean

all: emulator

emulator: $(OBJDIR)/cpu.o $(OBJDIR)/main_emu.o
	$(CC) $(CFLAGS) -o $@ $^

$(OBJDIR)/cpu.o: emulator/cpu.c include/sub_cpu.h | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/main_emu.o: emulator/main.c include/sub_cpu.h | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) emulator assembler
