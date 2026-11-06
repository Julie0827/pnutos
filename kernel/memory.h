#pragma once

#include <stdint.h>

struct memory_size {
  uint64_t val;
  const char *unit;
};

void memory_init(uint64_t base, uint64_t size);
uint64_t memory_get_end();
struct memory_size memory_get_formatted_size();
void memory_print_info(const char *prefix);
