#include "kassert.h"
#include "test.h"
#include "timer.h"

static void test_timer_ms() {
  uint64_t start = timer_get_ms();
  uint64_t now = start;

  while (now < start + 100) {
    now = timer_get_ms();
  }

  KASSERT(now >= start + 100);
}

static void test_timer_interrupt() {
  uint64_t start = timer_get_interrupt_count();
  uint64_t now = start;

  while (now < start + 10) {
    now = timer_get_interrupt_count();
  }

  KASSERT(now >= start + 10);
}

void test_timer() {
  test_timer_ms();
  test_timer_interrupt();
}
