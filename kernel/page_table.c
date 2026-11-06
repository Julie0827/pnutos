#include "page_table.h"
#include "kassert.h"

#define PAGE_TABLE_LEVEL_COUNT 3
#define PAGE_TABLE_NON_ROOT_LEVEL_COUNT (PAGE_TABLE_LEVEL_COUNT - 1)

#define PAGE_OFFSET_BIT_COUNT 12

#define VPN_PART_BIT_COUNT 9
#define VPN_PART_MASK 0x1FF

#define PTE_PPN_SHIFT 10
#define PTE_PPN_BIT_COUNT 44
#define PTE_PPN_MASK ((1ULL << PTE_PPN_BIT_COUNT) - 1)

#define PTE_V 0x01
#define PTE_LEAF_MASK (PTE_R | PTE_X)

struct page_table_link {
  page_table_t *table;
  page_table_t *parent;
  size_t pte_index;
};

page_table_t *page_table_alloc() {
  return page_alloc();
}

static size_t vpn_part(uintptr_t vaddr, size_t level) {
  return (vaddr >> (PAGE_OFFSET_BIT_COUNT + VPN_PART_BIT_COUNT * level)) & VPN_PART_MASK;
}

static size_t level_page_size(size_t level) {
  return (size_t)1 << (PAGE_OFFSET_BIT_COUNT + VPN_PART_BIT_COUNT * level);
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

static page_table_t *page_table_create_intermediate(pte_t *pte) {
  page_table_t *table = page_table_alloc();

  if (!table) return NULL;

  *pte = make_pte(paddr_to_ppn((uintptr_t)table), PTE_V);

  return table;
}

static void page_table_rollback(pte_t **ptes, size_t count) {
  while (count > 0) {
    pte_t *pte = ptes[--count];
    page_table_t *table = (page_table_t *)ppn_to_paddr(pte_ppn(*pte));

    *pte = 0;
    page_free(table);
  }
}

static pte_t *page_table_walk(page_table_t *root, uintptr_t vaddr, size_t level, bool create) {
  page_table_t *table = root;

  pte_t *created_ptes[PAGE_TABLE_NON_ROOT_LEVEL_COUNT];
  size_t created_count = 0;

  for (size_t l = PAGE_TABLE_LEVEL_COUNT - 1; l > level; l--) {
    pte_t *pte = &(*table)[vpn_part(vaddr, l)];

    if (!(*pte & PTE_V)) {
      if (!create) return NULL;

      if (!page_table_create_intermediate(pte)) {
        page_table_rollback(created_ptes, created_count);

        return NULL;
      }

      created_ptes[created_count++] = pte;
    }

    if (*pte & (PTE_R | PTE_W | PTE_X)) return NULL;

    table = (page_table_t *)ppn_to_paddr(pte_ppn(*pte));
  }

  return &(*table)[vpn_part(vaddr, level)];
}

static pte_t *page_table_find_leaf(page_table_t *root, uintptr_t vaddr, size_t *level,
                                   struct page_table_link *links) {
  page_table_t *table = root;

  for (size_t l = PAGE_TABLE_LEVEL_COUNT; l-- > 0;) {
    size_t index = vpn_part(vaddr, l);
    pte_t *pte = &(*table)[index];

    if (!(*pte & PTE_V)) return NULL;

    if (*pte & PTE_LEAF_MASK) {
      *level = l;

      return pte;
    }

    if (l == 0) return NULL;

    page_table_t *child = (page_table_t *)ppn_to_paddr(pte_ppn(*pte));

    if (links) {
      links[l - 1] = (struct page_table_link){
          .table = child,
          .parent = table,
          .pte_index = index,
      };
    }

    table = child;
  }

  return NULL;
}

bool page_table_map(page_table_t *root, uintptr_t vaddr, uintptr_t paddr, size_t level,
                    pte_t flags) {
  if (level >= PAGE_TABLE_LEVEL_COUNT) return false;

  size_t page_size = level_page_size(level);

  if (vaddr % page_size != 0) return false;
  if (paddr % page_size != 0) return false;

  if (!(flags & PTE_R) && flags & PTE_W) return false;
  if (!(flags & PTE_LEAF_MASK)) return false;

  pte_t *pte = page_table_walk(root, vaddr, level, true);

  if (!pte) return false;
  if (*pte & PTE_V) return false;

  *pte = make_pte(paddr_to_ppn(paddr), flags | PTE_V);

  return true;
}

bool page_table_map_range(page_table_t *root, uintptr_t vaddr, uintptr_t paddr, size_t size,
                          size_t level, pte_t flags) {
  if (level >= PAGE_TABLE_LEVEL_COUNT) return false;
  if (size == 0) return true;
  if (size - 1 > UINTPTR_MAX - vaddr) return false;
  if (size - 1 > UINTPTR_MAX - paddr) return false;

  size_t page_size = level_page_size(level);

  if (size % page_size != 0) return false;

  for (size_t i = 0; i < size / page_size; i++) {
    if (!page_table_map(root, vaddr + page_size * i, paddr + page_size * i, level, flags)) {
      page_table_unmap_range(root, vaddr, page_size * i);

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

static void page_table_prune(size_t level, struct page_table_link *links) {
  for (size_t l = level; l < PAGE_TABLE_NON_ROOT_LEVEL_COUNT; l++) {
    struct page_table_link *link = &links[l];

    if (!page_table_is_empty(link->table)) return;

    (*link->parent)[link->pte_index] = 0;

    page_free(link->table);
  }
}

bool page_table_unmap(page_table_t *root, uintptr_t vaddr) {
  size_t level;
  struct page_table_link links[PAGE_TABLE_NON_ROOT_LEVEL_COUNT];

  pte_t *pte = page_table_find_leaf(root, vaddr, &level, links);

  if (!pte) return false;
  if (vaddr % level_page_size(level) != 0) return false;

  *pte = 0;

  page_table_prune(level, links);

  return true;
}

static bool page_table_unmap_subtree(page_table_t *table, uintptr_t vaddr, size_t size,
                                     size_t level) {
  size_t first = vpn_part(vaddr, level);
  size_t last = vpn_part(vaddr + size - 1, level);

  size_t page_size = level_page_size(level);
  uintptr_t first_region_start = vaddr & ~(page_size - 1);
  uintptr_t range_last = vaddr + size - 1;

  for (size_t i = first; i <= last; i++) {
    pte_t *pte = &(*table)[i];

    KASSERT(*pte & PTE_V);

    if (*pte & PTE_LEAF_MASK) {
      *pte = 0;
      continue;
    }

    KASSERT(level > 0);

    page_table_t *child = (page_table_t *)ppn_to_paddr(pte_ppn(*pte));

    uintptr_t region_start = first_region_start + (i - first) * page_size;
    uintptr_t region_last = region_start + page_size - 1;

    uintptr_t child_vaddr = vaddr > region_start ? vaddr : region_start;
    uintptr_t child_last = range_last < region_last ? range_last : region_last;

    size_t child_size = child_last - child_vaddr + 1;

    bool empty = page_table_unmap_subtree(child, child_vaddr, child_size, level - 1);

    if (empty) {
      *pte = 0;
      page_free(child);
    }
  }

  return page_table_is_empty(table);
}

bool page_table_unmap_range(page_table_t *root, uintptr_t vaddr, size_t size) {
  if (size == 0) return true;
  if (size - 1 > UINTPTR_MAX - vaddr) return false;

  size_t validated = 0;

  while (size > validated) {
    size_t level;
    uintptr_t curr_vaddr = vaddr + validated;
    pte_t *pte = page_table_find_leaf(root, curr_vaddr, &level, NULL);

    if (!pte) return false;

    size_t page_size = level_page_size(level);

    if (curr_vaddr % page_size != 0) return false;
    if (page_size > size - validated) return false;

    validated += page_size;
  }

  page_table_unmap_subtree(root, vaddr, size, PAGE_TABLE_LEVEL_COUNT - 1);

  return true;
}

bool page_table_translate(page_table_t *root, uintptr_t vaddr, uintptr_t *paddr) {
  size_t level;
  pte_t *pte = page_table_find_leaf(root, vaddr, &level, NULL);

  if (!pte) return false;

  uintptr_t base = ppn_to_paddr(pte_ppn(*pte));
  size_t page_size = level_page_size(level);

  if (base % page_size != 0) return false;

  *paddr = base | (vaddr & (page_size - 1));

  return true;
}

static void page_table_free_subtree(page_table_t *table, size_t level) {
  if (level > 0) {
    for (size_t i = 0; i < PAGE_TABLE_ENTRY_COUNT; i++) {
      pte_t *pte = &(*table)[i];

      if (!(*pte & PTE_V)) continue;
      if (*pte & PTE_LEAF_MASK) continue;

      page_table_free_subtree((page_table_t *)ppn_to_paddr(pte_ppn(*pte)), level - 1);
    }
  }

  page_free(table);
}

void page_table_free(page_table_t *root) {
  if (!root) return;

  page_table_free_subtree(root, PAGE_TABLE_LEVEL_COUNT - 1);
}
