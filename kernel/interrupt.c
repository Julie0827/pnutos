#include "interrupt.h"

#include <stdint.h>

#define SSTATUS_SIE (1ULL << 1)

#define SIE_STIE (1ULL << 5)
#define SIE_SEIE (1ULL << 9)

void interrupt_enable() {
  __asm__ volatile("csrs sstatus, %0" : : "r"(SSTATUS_SIE));
}

bool interrupt_disable() {
  uint64_t sstatus;

  __asm__ volatile("csrrc %0, sstatus, %1" : "=r"(sstatus) : "r"(SSTATUS_SIE));

  return sstatus & SSTATUS_SIE;
}

void interrupt_restore(bool was_enabled) {
  if (was_enabled) interrupt_enable();
}

static void sie_set(uint64_t mask) {
  __asm__ volatile("csrs sie, %0" : : "r"(mask));
}

void interrupt_enable_timer() {
  sie_set(SIE_STIE);
}

void interrupt_enable_external() {
  sie_set(SIE_SEIE);
}
