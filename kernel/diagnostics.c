#include "diagnostics.h"
#include "ansi.h"
#include "config.h"
#include "kprintf.h"
#include "memory.h"

static const char boot_prefix[] = ANSI_BLUE "[boot]" ANSI_RESET " ";
static const char diag_prefix[] = ANSI_BLUE "[diag]" ANSI_RESET " ";

void diagnostics_print(unsigned long hart_id) {
#if CONFIG_DIAGNOSTICS
  kprintf("%shart: %lu\n", boot_prefix, hart_id);

  memory_print_info(diag_prefix);
#endif
}
