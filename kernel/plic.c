#include "plic.h"
#include "uart.h"

#define PLIC_BASE 0x0C000000UL

#define PLIC_PRIORITY(id) (PLIC_BASE + 4 * (id))

#define PLIC_ENABLE(context, id) (PLIC_BASE + 0x2000 + 0x80 * (context) + 4 * ((id) / 32))
#define PLIC_ENABLE_BIT(id) (1U << ((id) % 32))

#define PLIC_THRESHOLD(context) (PLIC_BASE + 0x200000 + 0x1000 * (context))

#define PLIC_CLAIM(context) (PLIC_BASE + 0x200004 + 0x1000 * (context))
#define PLIC_COMPLETE(context) PLIC_CLAIM(context)

#define PLIC_HART0_S 1

#define UART_IRQ 10

static uint32_t plic_read(uintptr_t addr) {
  return *(volatile uint32_t *)addr;
}

static void plic_write(uintptr_t addr, uint32_t val) {
  *(volatile uint32_t *)addr = val;
}

static void plic_set_priority(uint32_t id, uint32_t priority) {
  plic_write(PLIC_PRIORITY(id), priority);
}

static void plic_enable(uint32_t context, uint32_t id) {
  uintptr_t addr = PLIC_ENABLE(context, id);
  uint32_t enabled = plic_read(addr);

  enabled |= PLIC_ENABLE_BIT(id);

  plic_write(addr, enabled);
}

static void plic_set_threshold(uint32_t context, uint32_t threshold) {
  plic_write(PLIC_THRESHOLD(context), threshold);
}

void plic_init() {
  plic_set_threshold(PLIC_HART0_S, 0);
  plic_set_priority(UART_IRQ, 1);
  plic_enable(PLIC_HART0_S, UART_IRQ);
}

static uint32_t plic_claim(uint32_t context) {
  return plic_read(PLIC_CLAIM(context));
}

static void plic_complete(uint32_t context, uint32_t id) {
  plic_write(PLIC_COMPLETE(context), id);
}

void plic_handle_interrupt() {
  uint32_t id = plic_claim(PLIC_HART0_S);

  if (!id) return;

  switch (id) {
    case UART_IRQ:
      uart_handle_interrupt();
      break;
  }

  plic_complete(PLIC_HART0_S, id);
}
