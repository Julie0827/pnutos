#pragma once

#include "page.h"

#include <stddef.h>
#include <stdint.h>

#define PTE_R (1 << 1)
#define PTE_W (1 << 2)
#define PTE_X (1 << 3)
#define PTE_U (1 << 4)
#define PTE_G (1 << 5)
#define PTE_A (1 << 6)
#define PTE_D (1 << 7)

typedef uint64_t pte_t;

#define PAGE_TABLE_ENTRY_COUNT (PAGE_SIZE / sizeof(pte_t))

typedef pte_t page_table_t[PAGE_TABLE_ENTRY_COUNT];

page_table_t *page_table_alloc();

bool page_table_map(page_table_t *root, uintptr_t vaddr, uintptr_t paddr, pte_t flags);
bool page_table_map_range(page_table_t *root, uintptr_t vaddr, uintptr_t paddr, size_t size,
                          pte_t flags);

bool page_table_unmap(page_table_t *root, uintptr_t vaddr);
bool page_table_unmap_range(page_table_t *root, uintptr_t vaddr, size_t size);

bool page_table_translate(page_table_t *root, uintptr_t vaddr, uintptr_t *paddr);

void page_table_free(page_table_t *root);
