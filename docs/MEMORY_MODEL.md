# SUB64 Memory Ordering Model (SUB-MO)

SUB64 uses a **custom memory model** called **SUB-MO**, positioned between x86 TSO (strong) and ARM/RISC-V weak ordering.

---

## Ordering Rules

1. **Load–Load**: Ordered within a single thread.
2. **Store–Store**: Ordered within a single thread.
3. **Load–Store**: Ordered (no store-load reordering within thread).
4. **Store–Load**: **NOT** ordered by default — use `FENCE` to enforce.
5. **Atomic ops** (`XCHG`, `CAS`): Full sequential consistency.

## FENCE Variants

| Instruction | Meaning |
|-------------|--------|
| `FENCE` | Full barrier (all loads and stores) |
| `FENCE.LD` | Load-only barrier |
| `FENCE.ST` | Store-only barrier |
| `FENCE.TSO` | x86-TSO compatible barrier |

## Atomic Operations

- `XCHG Rd, Ra` — atomic exchange: Rd ↔ MEM[Ra]
- `CAS Rd, Ra, Rb` — compare-and-swap: if MEM[Ra]==Rd, MEM[Ra]=Rb, else Rd=MEM[Ra]

All atomics are **acquire-release** by default.

---

*SUB64 Memory Model v1.0 — GPL v3*
