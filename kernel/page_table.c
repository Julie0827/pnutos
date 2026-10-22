#include "page_table.h"

#define PAGE_TABLE_LEVEL_COUNT 3
#define PAGE_TABLE_NON_ROOT_LEVEL_COUNT (PAGE_TABLE_LEVEL_COUNT - 1)

#define PAGE_OFFSET_BIT_COUNT 12

#define VPN_PART_BIT_COUNT 9
#define VPN_PART_MASK 0x1FF

#define PTE_PPN_SHIFT 10
#define PTE_PPN_BIT_COUNT 44
#define PTE_PPN_MASK ((1ULL << PTE_PPN_BIT_COUNT) - 1)

#define PTE_V 0x01

page_table_t *page_table_alloc() {
  return page_alloc();
}

static size_t vpn_part(uintptr_t vaddr, size_t level) {
  return (vaddr >> (PAGE_OFFSET_BIT_COUNT + VPN_PART_BIT_COUNT * level)) & VPN_PART_MASK;
}

static uintptr_t ppn_to_paddr(uint64_t ppn) {
  return ppn << PAGE_OFFSET_BIT_COUNT;
}

static uint64_t paddr_to_ppn(uintptr_t paddr) {
  return paddr >> PAGE_OFFSET_BIT_COUNT;
}

static uint64_t pte_ppn(pte_t pte) {
  return (pte >> PTE_PPN_SHIFT) & PTE_PPN_MASK;
}

static pte_t make_pte(uint64_t ppn, pte_t flags) {
  return (ppn << PTE_PPN_SHIFT) | flags;
}

static bool page_table_create_intermediate(pte_t *pte, page_table_t **created_table,
                                           pte_t **created_pte) {
  page_table_t *table = page_table_alloc();

  if (!table) {
    if (*created_table) {
      **created_pte = 0;
      page_free(*created_table);
    }

    return false;
  }

  *pte = make_pte(paddr_to_ppn((uintptr_t)table), PTE_V);

  if (!*created_table) {
    *created_table = table;
    *created_pte = pte;
  }

  return true;
}

static pte_t *page_table_walk(page_table_t *root, uintptr_t vaddr, bool create,
                              page_table_t **tables) {
  page_table_t *table = root;

  page_table_t *created_table = NULL;
  pte_t *created_pte = NULL;

  for (size_t level = PAGE_TABLE_LEVEL_COUNT - 1; level > 0; level--) {
    pte_t *pte = &(*table)[vpn_part(vaddr, level)];

    if (!(*pte & PTE_V)) {
      if (!create) return NULL;
      if (!page_table_create_intermediate(pte, &created_table, &created_pte)) return NULL;
    }

    // TODO: Support superpage mappings (2 MiB and 1 GiB)
    if (*pte & (PTE_R | PTE_W | PTE_X)) return NULL;

    table = (page_table_t *)ppn_to_paddr(pte_ppn(*pte));

    if (tables) tables[level - 1] = table;
  }

  return &(*table)[vpn_part(vaddr, 0)];
}

bool page_table_map(page_table_t *root, uintptr_t vaddr, uintptr_t paddr, pte_t flags) {
  if (vaddr % PAGE_SIZE != 0) return false;
  if (paddr % PAGE_SIZE != 0) return false;

  if (!(flags & PTE_R) && flags & PTE_W) return false;

  pte_t *pte = page_table_walk(root, vaddr, true, NULL);

  if (!pte) return false;
  if (*pte & PTE_V) return false;

  *pte = make_pte(paddr_to_ppn(paddr), flags | PTE_V);

  return true;
}

bool page_table_map_range(page_table_t *root, uintptr_t vaddr, uintptr_t paddr, size_t size,
                          pte_t flags) {
  if (size % PAGE_SIZE != 0) return false;

  size_t count = size / PAGE_SIZE;

  for (size_t i = 0; i < count; i++) {
    if (!page_table_map(root, vaddr + PAGE_SIZE * i, paddr + PAGE_SIZE * i, flags)) {
      page_table_unmap_range(root, vaddr, PAGE_SIZE * i);
      return false;
    }
  }

  return true;
}

static bool page_table_is_empty(page_table_t *table) {
  for (size_t i = 0; i < PAGE_TABLE_ENTRY_COUNT; i++) {
    if ((*table)[i] & PTE_V) return false;
  }

  return true;
}

static void page_table_prune(page_table_t *root, uintptr_t vaddr, page_table_t **tables) {
  for (size_t level = 0; level < PAGE_TABLE_NON_ROOT_LEVEL_COUNT; level++) {
    page_table_t *table = tables[level];
    page_table_t *parent = level == 0 ? tables[level + 1] : root;

    if (!page_table_is_empty(table)) return;

    (*parent)[vpn_part(vaddr, level + 1)] = 0;

    page_free(table);
  }
}

bool page_table_unmap(page_table_t *root, uintptr_t vaddr) {
  if (vaddr % PAGE_SIZE != 0) return false;

  page_table_t *tables[PAGE_TABLE_NON_ROOT_LEVEL_COUNT];

  pte_t *pte = page_table_walk(root, vaddr, false, tables);

  if (!pte) return false;
  if (!(*pte & PTE_V)) return false;

  *pte = 0;

  page_table_prune(root, vaddr, tables);

  return true;
}

// TODO: Optimize pruning for range unmapping
bool page_table_unmap_range(page_table_t *root, uintptr_t vaddr, size_t size) {
  if (size % PAGE_SIZE != 0) return false;

  size_t count = size / PAGE_SIZE;

  for (size_t i = 0; i < count; i++) {
    if (!page_table_unmap(root, vaddr + PAGE_SIZE * i)) return false;
  }

  return true;
}

bool page_table_translate(page_table_t *root, uintptr_t vaddr, uintptr_t *paddr) {
  pte_t *pte = page_table_walk(root, vaddr, false, NULL);

  if (!pte) return false;
  if (!(*pte & PTE_V)) return false;
  if (!(*pte & (PTE_R | PTE_X))) return false;

  *paddr = ppn_to_paddr(pte_ppn(*pte)) | (vaddr & (PAGE_SIZE - 1));

  return true;
}

static void page_table_free_subtree(page_table_t *table, size_t level) {
  if (level > 0) {
    for (size_t i = 0; i < PAGE_TABLE_ENTRY_COUNT; i++) {
      pte_t *pte = &(*table)[i];

      if (!(*pte & PTE_V)) continue;
      if (*pte & (PTE_R | PTE_W | PTE_X)) continue;

      page_table_free_subtree((page_table_t *)ppn_to_paddr(pte_ppn(*pte)), level - 1);
    }
  }

  page_free(table);
}

void page_table_free(page_table_t *root) {
  if (!root) return;

  page_table_free_subtree(root, PAGE_TABLE_LEVEL_COUNT - 1);
}
