# SUB64 Application Binary Interface (ABI)

The **SUB ABI** defines the calling convention and system call interface for SUB64.

---

## Calling Convention

### Argument Passing
- First 4 integer/pointer arguments: **X4, X5, X6, X7**
- First 4 floating-point arguments: **V0, V1, V2, V3**
- Additional arguments: pushed on stack, right-to-left

### Return Values
- Integer/pointer: **X4** (second in X5)
- Float/vector: **V0**

### Register Ownership

| Registers | Owner | Meaning |
|-----------|-------|---------|
| X0 | — | Always zero |
| X1 | Callee-saved | Return address (LR alias) |
| X2 | Callee-saved | Stack pointer |
| X3 | Callee-saved | Frame pointer |
| X4–X7 | Caller-saved | Args / return values |
| X8–X15 | Caller-saved | Temporaries |
| X16–X23 | Callee-saved | Saved registers |
| V0–V3 | Caller-saved | FP/vector args |
| V4–V7 | Callee-saved | FP/vector saved |

### Stack Layout (frame)
```
   High addr
   ┌────────────────┐
   │  Caller args   │  (if > 4 args)
   ├────────────────┤ ← SP before CALL
   │   Return addr  │  (saved LR)
   │   Saved FP     │
   ├────────────────┤ ← FP
   │  Local vars    │
   ├────────────────┤ ← SP (current)
   Low addr
```

Stack is **16-byte aligned** at all CALL sites.

---

## System Call Convention

- Syscall number in **X8**
- Arguments in **X4–X7** (up to 4)
- Invoke with `SYSCALL` instruction
- Return value in **X4**, error code in **X5** (0 = success)

### Standard Syscall Table

| Number | Name | Description |
|--------|------|-------------|
| 0 | sys_exit | Terminate process |
| 1 | sys_write | Write to file descriptor |
| 2 | sys_read | Read from file descriptor |
| 3 | sys_open | Open file |
| 4 | sys_close | Close file descriptor |
| 5 | sys_mmap | Map memory |
| 6 | sys_munmap | Unmap memory |
| 7 | sys_getpid | Get process ID |
| 8 | sys_fork | Fork process |
| 9 | sys_exec | Execute program |

---

*SUB64 ABI v1.0 — GPL v3 — Copyright (C) 2026 Subho*
