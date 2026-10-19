# Interrupts

PnutOS provides global S-mode interrupt control using the `SIE` (Supervisor Interrupt Enable) bit in the `sstatus` CSR. Individual interrupt types are enabled using the `sie` CSR.

## Global Interrupt State

- `SIE = 0` - interrupts are disabled
- `SIE = 1` - interrupts are enabled

## `sie` Bits

| Bit | Name | Purpose |
|---|---|---|
| `STIE` | Supervisor Timer Interrupt Enable | Enable timer interrupts |
| `SEIE` | Supervisor External Interrupt Enable | Enable external interrupts |

## Software Interface

```c
void interrupt_enable();
bool interrupt_disable();
void interrupt_restore(bool was_enabled);
void interrupt_enable_timer();
void interrupt_enable_external();
```

- `interrupt_enable()` - enables interrupts
- `interrupt_disable()` - disables interrupts and returns the previous interrupt state
- `interrupt_restore()` - restores the previous interrupt state
- `interrupt_enable_timer()` - enables timer interrupts
- `interrupt_enable_external()` - enables external interrupts
