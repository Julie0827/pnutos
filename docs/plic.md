# PLIC

PnutOS uses the RISC-V Platform-Level Interrupt Controller (PLIC) to handle external interrupts.

## Configuration

- Base address: `0x0C000000`
- Context: Hart 0 Supervisor mode
- UART interrupt ID: `10`
- UART priority: `1`
- Priority threshold: `0`

## Interrupt Flow

When a device raises an external interrupt, the PLIC determines whether the interrupt is eligible based on the source's priority, its enable state for the context, and the context's priority threshold.

When the CPU takes the external interrupt, control is transferred to the trap handler, which calls the PLIC interrupt handler. The PLIC interrupt handler claims the interrupt, and the PLIC returns the ID of the highest-priority eligible pending interrupt.

The interrupt ID is used to dispatch the interrupt to the appropriate device handler. After the interrupt is handled, the PLIC interrupt handler sends the ID back to the PLIC to complete the interrupt.

## Software Interface

```c
void plic_init();
void plic_handle_interrupt();
```

- `plic_init()` - initializes the PLIC
- `plic_handle_interrupt()` - claims, dispatches, and completes an external interrupt
