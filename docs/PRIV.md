# SUB64 Privilege & Interrupt Model

## Privilege Rings

SUB64 has **3 privilege levels**:

| Ring | Name | Purpose |
|------|------|---------|
| 0 | Kernel | OS kernel, full access |
| 1 | User | Application code |
| 2 | Guest | Hypervisor sandbox |

Current ring stored in special register `RING` (read-only from Ring 1+).

## Ring Transitions

| Transition | Instruction | Notes |
|------------|-------------|-------|
| 1 → 0 | `SYSCALL` | Saves PC→UVEC, jumps to KVEC |
| 0 → 1 | `SYSRET` | Restores from UVEC |
| Any → 0 | Hardware IRQ | Saves context to IVT entry |

## Interrupt Vector Table (IVT)

Base address stored in `KVEC` register. Each entry is 16 bytes.

| Vector | Exception |
|--------|-----------|
| 0 | Reset |
| 1 | Illegal instruction |
| 2 | Page fault (read) |
| 3 | Page fault (write) |
| 4 | Alignment fault |
| 5 | Arithmetic trap (div by zero) |
| 6 | Syscall |
| 7–31 | Hardware IRQ 0–24 |
| 32–63 | Software interrupts |

## Protected Instructions

The following instructions are **Ring 0 only** and fault if executed from Ring 1/2:
- Write to `KVEC`, `RING`, `UVEC`
- `SYSRET`
- Direct MMIO access
- `FENCE.TSO`

---

*SUB64 Privilege Model v1.0 — GPL v3*
