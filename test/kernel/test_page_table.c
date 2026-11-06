#include "kernel/kassert.h"
#include "kernel/page_table.h"
#include "test.h"

#define PAGE_TABLE_LEVEL_COUNT 3

#define KIB ((size_t)1024)
#define MIB (KIB * KIB)
#define GIB (MIB * KIB)

static const size_t LEVEL_PAGE_SIZES[] = {4 * KIB, 2 * MIB, GIB};

static_assert(sizeof(LEVEL_PAGE_SIZES) / sizeof(LEVEL_PAGE_SIZES[0]) == PAGE_TABLE_LEVEL_COUNT);

#define VADDR ((uintptr_t)0x40000000)
#define PADDR ((uintptr_t)0x80000000)

static_assert(VADDR % GIB == 0);
static_assert(PADDR % GIB == 0);

#define RW (PTE_R | PTE_W)

static page_table_t *new_root() {
  page_table_t *root = page_table_alloc();

  KASSERT(root);

  for (size_t i = 0; i < PAGE_TABLE_ENTRY_COUNT; i++) {
    KASSERT(!(*root)[i]);
  }

  return root;
}

static void expect_translate(page_table_t *root, uintptr_t vaddr, uintptr_t expected) {
  uintptr_t paddr;

  KASSERT(page_table_translate(root, vaddr, &paddr));
  KASSERT(paddr == expected);
}

static void expect_unmapped(page_table_t *root, uintptr_t vaddr) {
  uintptr_t paddr;

  KASSERT(!page_table_translate(root, vaddr, &paddr));
}

static void test_map_translate() {
  for (size_t level = 0; level < PAGE_TABLE_LEVEL_COUNT; level++) {
    page_table_t *root = new_root();

    size_t page_size = LEVEL_PAGE_SIZES[level];

    KASSERT(page_table_map(root, VADDR, PADDR, level, RW));

    expect_translate(root, VADDR, PADDR);
    expect_translate(root, VADDR + 0x123, PADDR + 0x123);
    expect_translate(root, VADDR + page_size - 1, PADDR + page_size - 1);

    page_table_free(root);
  }
}

static void test_aliasing() {
  for (size_t level = 0; level < PAGE_TABLE_LEVEL_COUNT; level++) {
    page_table_t *root = new_root();

    size_t page_size = LEVEL_PAGE_SIZES[level];

    KASSERT(page_table_map(root, VADDR, PADDR, level, RW));
    KASSERT(page_table_map(root, VADDR + page_size, PADDR, level, RW));

    expect_translate(root, VADDR, PADDR);
    expect_translate(root, VADDR + page_size, PADDR);

    expect_translate(root, VADDR + page_size + 0x123, PADDR + 0x123);

    page_table_free(root);
  }
}

static void test_map_rejects_invalid_flags() {
  page_table_t *root = new_root();

  KASSERT(!page_table_map(root, VADDR, PADDR, 0, 0));
  KASSERT(!page_table_map(root, VADDR, PADDR, 0, PTE_W));
  KASSERT(!page_table_map(root, VADDR, PADDR, 0, PTE_W | PTE_X));

  expect_unmapped(root, VADDR);

  page_table_free(root);
}

static void test_map_rejects_double_map() {
  for (size_t level = 0; level < PAGE_TABLE_LEVEL_COUNT; level++) {
    page_table_t *root = new_root();

    KASSERT(page_table_map(root, VADDR, PADDR, level, RW));
    KASSERT(!page_table_map(root, VADDR, PADDR + LEVEL_PAGE_SIZES[level], level, RW));

    expect_translate(root, VADDR, PADDR);

    page_table_free(root);
  }
}

static void test_map_rejects_misaligned() {
  for (size_t level = 0; level < PAGE_TABLE_LEVEL_COUNT; level++) {
    page_table_t *root = new_root();

    KASSERT(!page_table_map(root, VADDR + 1, PADDR, level, RW));
    KASSERT(!page_table_map(root, VADDR, PADDR + 1, level, RW));

    for (size_t i = 0; i < level; i++) {
      KASSERT(!page_table_map(root, VADDR + LEVEL_PAGE_SIZES[i], PADDR, level, RW));
      KASSERT(!page_table_map(root, VADDR, PADDR + LEVEL_PAGE_SIZES[i], level, RW));
    }

    expect_unmapped(root, VADDR);

    page_table_free(root);
  }
}

static void test_map_rejects_invalid_level() {
  page_table_t *root = new_root();

  KASSERT(!page_table_map(root, VADDR, PADDR, PAGE_TABLE_LEVEL_COUNT, RW));
  KASSERT(!page_table_map_range(root, VADDR, PADDR, PAGE_SIZE, PAGE_TABLE_LEVEL_COUNT, RW));

  page_table_free(root);
}

static void test_unmap() {
  for (size_t level = 0; level < PAGE_TABLE_LEVEL_COUNT; level++) {
    page_table_t *root = new_root();

    KASSERT(page_table_map(root, VADDR, PADDR, level, RW));
    KASSERT(page_table_unmap(root, VADDR));

    expect_unmapped(root, VADDR);

    page_table_free(root);
  }
}

static void test_unmap_prune() {
  for (size_t level = 0; level < PAGE_TABLE_LEVEL_COUNT; level++) {
    page_table_t *root = new_root();

    size_t initial_free_count = page_free_count();
    size_t intermediate_count = PAGE_TABLE_LEVEL_COUNT - level - 1;

    KASSERT(page_table_map(root, VADDR, PADDR, level, RW));
    KASSERT(page_free_count() == initial_free_count - intermediate_count);

    KASSERT(page_table_unmap(root, VADDR));
    KASSERT(page_free_count() == initial_free_count);

    page_table_free(root);
  }
}

static void test_unmap_prune_nonempty() {
  page_table_t *root = new_root();

  size_t initial_free_count = page_free_count();
  size_t intermediate_count = PAGE_TABLE_LEVEL_COUNT - 1;
  size_t expected_free_count = initial_free_count - intermediate_count;

  KASSERT(page_table_map(root, VADDR, PADDR, 0, RW));
  KASSERT(page_free_count() == expected_free_count);

  KASSERT(page_table_map(root, VADDR + PAGE_SIZE, PADDR + PAGE_SIZE, 0, RW));
  KASSERT(page_free_count() == expected_free_count);

  KASSERT(page_table_unmap(root, VADDR));
  KASSERT(page_free_count() == expected_free_count);

  expect_translate(root, VADDR + PAGE_SIZE, PADDR + PAGE_SIZE);

  KASSERT(page_table_unmap(root, VADDR + PAGE_SIZE));
  KASSERT(page_free_count() == initial_free_count);

  page_table_free(root);
}

static void test_unmap_rejects_unmapped() {
  page_table_t *root = new_root();

  KASSERT(!page_table_unmap(root, VADDR));

  page_table_free(root);
}

static void test_unmap_rejects_misaligned() {
  for (size_t level = 0; level < PAGE_TABLE_LEVEL_COUNT; level++) {
    page_table_t *root = new_root();

    KASSERT(page_table_map(root, VADDR, PADDR, level, RW));
    KASSERT(!page_table_unmap(root, VADDR + 1));

    for (size_t i = 0; i < level; i++) {
      KASSERT(!page_table_unmap(root, VADDR + LEVEL_PAGE_SIZES[i]));
    }

    expect_translate(root, VADDR, PADDR);

    page_table_free(root);
  }
}

static void test_map_range() {
  for (size_t level = 0; level < PAGE_TABLE_LEVEL_COUNT; level++) {
    page_table_t *root = new_root();

    size_t page_size = LEVEL_PAGE_SIZES[level];
    size_t page_count = 4;

    KASSERT(page_table_map_range(root, VADDR, PADDR, page_count * page_size, level, RW));

    for (size_t i = 0; i < page_count; i++) {
      uintptr_t vaddr = VADDR + i * page_size;
      uintptr_t paddr = PADDR + i * page_size;

      expect_translate(root, vaddr, paddr);
      expect_translate(root, vaddr + page_size - 1, paddr + page_size - 1);
    }

    page_table_free(root);
  }
}

static void test_map_range_atomicity() {
  for (size_t level = 0; level < PAGE_TABLE_LEVEL_COUNT; level++) {
    page_table_t *root = new_root();

    size_t page_size = LEVEL_PAGE_SIZES[level];
    size_t page_count = 4;

    size_t conflict_index = page_count - 1;
    uintptr_t conflict_vaddr = VADDR + conflict_index * page_size;

    KASSERT(page_table_map(root, conflict_vaddr, PADDR, level, RW));
    KASSERT(!page_table_map_range(root, VADDR, PADDR, page_count * page_size, level, RW));

    for (size_t i = 0; i < page_count; i++) {
      if (i != conflict_index) expect_unmapped(root, VADDR + i * page_size);
    }

    expect_translate(root, conflict_vaddr, PADDR);

    page_table_free(root);
  }
}

static void test_map_range_reclamation() {
  page_table_t *root = new_root();

  uintptr_t vaddr = VADDR - PAGE_SIZE;
  size_t page_count = 2;

  size_t conflict_index = page_count - 1;
  uintptr_t conflict_vaddr = vaddr + conflict_index * PAGE_SIZE;

  size_t initial_free_count = page_free_count();
  size_t intermediate_count = PAGE_TABLE_LEVEL_COUNT - 1;
  size_t expected_free_count = initial_free_count - intermediate_count;

  KASSERT(page_table_map(root, conflict_vaddr, PADDR, 0, RW));
  KASSERT(page_free_count() == expected_free_count);

  KASSERT(!page_table_map_range(root, vaddr, PADDR, page_count * PAGE_SIZE, 0, RW));
  KASSERT(page_free_count() == expected_free_count);

  for (size_t i = 0; i < page_count; i++) {
    if (i != conflict_index) expect_unmapped(root, vaddr + i * PAGE_SIZE);
  }

  expect_translate(root, conflict_vaddr, PADDR);

  page_table_free(root);
}

static void test_map_range_rejects_invalid_size() {
  for (size_t level = 0; level < PAGE_TABLE_LEVEL_COUNT; level++) {
    page_table_t *root = new_root();

    KASSERT(!page_table_map_range(root, VADDR, PADDR, LEVEL_PAGE_SIZES[level] + 1, level, RW));

    expect_unmapped(root, VADDR);

    page_table_free(root);
  }
}

static void test_unmap_range() {
  for (size_t level = 0; level < PAGE_TABLE_LEVEL_COUNT; level++) {
    page_table_t *root = new_root();

    size_t page_size = LEVEL_PAGE_SIZES[level];
    size_t page_count = 4;

    size_t unmap_index = 1;
    size_t unmap_count = 2;

    KASSERT(page_table_map_range(root, VADDR, PADDR, page_count * page_size, level, RW));
    KASSERT(page_table_unmap_range(root, VADDR + unmap_index * page_size, unmap_count * page_size));

    for (size_t i = 0; i < page_count; i++) {
      uintptr_t vaddr = VADDR + i * page_size;
      uintptr_t paddr = PADDR + i * page_size;

      if (i >= unmap_index && i < unmap_index + unmap_count) expect_unmapped(root, vaddr);
      else expect_translate(root, vaddr, paddr);
    }

    page_table_free(root);
  }
}

static void test_unmap_range_atomicity() {
  for (size_t level = 0; level < PAGE_TABLE_LEVEL_COUNT; level++) {
    page_table_t *root = new_root();

    size_t page_size = LEVEL_PAGE_SIZES[level];
    size_t page_count = 4;
    size_t gap_index = page_count - 1;

    for (size_t i = 0; i < page_count; i++) {
      if (i != gap_index)
        KASSERT(page_table_map(root, VADDR + i * page_size, PADDR + i * page_size, level, RW));
    }

    KASSERT(!page_table_unmap_range(root, VADDR, page_count * page_size));

    for (size_t i = 0; i < page_count; i++) {
      uintptr_t vaddr = VADDR + i * page_size;
      uintptr_t paddr = PADDR + i * page_size;

      if (i == gap_index) expect_unmapped(root, vaddr);
      else expect_translate(root, vaddr, paddr);
    }

    page_table_free(root);
  }
}

static void test_unmap_range_prune() {
  for (size_t level = 0; level < PAGE_TABLE_LEVEL_COUNT; level++) {
    page_table_t *root = new_root();

    size_t initial_free_count = page_free_count();
    size_t intermediate_count = PAGE_TABLE_LEVEL_COUNT - level - 1;

    size_t page_size = LEVEL_PAGE_SIZES[level];
    size_t page_count = 4;
    size_t size = page_count * page_size;

    KASSERT(page_table_map_range(root, VADDR, PADDR, size, level, RW));
    KASSERT(page_free_count() == initial_free_count - intermediate_count);

    KASSERT(page_table_unmap_range(root, VADDR, size));
    KASSERT(page_free_count() == initial_free_count);

    page_table_free(root);
  }
}

static void test_unmap_range_prune_nonempty() {
  page_table_t *root = new_root();

  size_t initial_free_count = page_free_count();
  size_t intermediate_count = PAGE_TABLE_LEVEL_COUNT - 1;
  size_t expected_free_count = initial_free_count - intermediate_count;

  size_t map_count = 4;
  size_t unmap_count = 2;
  size_t unmap_size = unmap_count * PAGE_SIZE;

  KASSERT(page_table_map_range(root, VADDR, PADDR, map_count * PAGE_SIZE, 0, RW));
  KASSERT(page_free_count() == expected_free_count);

  KASSERT(page_table_unmap_range(root, VADDR, unmap_size));
  KASSERT(page_free_count() == expected_free_count);

  expect_translate(root, VADDR + unmap_size, PADDR + unmap_size);

  page_table_free(root);
}

static void test_unmap_range_prune_partial() {
  page_table_t *root = new_root();

  size_t initial_free_count = page_free_count();

  uintptr_t vaddr = VADDR + LEVEL_PAGE_SIZES[1] - PAGE_SIZE;

  KASSERT(page_table_map(root, vaddr, PADDR, 0, RW));
  KASSERT(page_table_map(root, vaddr + PAGE_SIZE, PADDR + PAGE_SIZE, 0, RW));
  KASSERT(page_free_count() == initial_free_count - PAGE_TABLE_LEVEL_COUNT);

  KASSERT(page_table_unmap_range(root, vaddr, PAGE_SIZE));
  KASSERT(page_free_count() == initial_free_count - PAGE_TABLE_LEVEL_COUNT + 1);

  expect_unmapped(root, vaddr);
  expect_translate(root, vaddr + PAGE_SIZE, PADDR + PAGE_SIZE);

  page_table_free(root);
}

static void test_unmap_range_rejects_partial_superpage() {
  page_table_t *root = new_root();

  uintptr_t vaddr = VADDR - PAGE_SIZE;
  uintptr_t paddr = PADDR - PAGE_SIZE;

  KASSERT(page_table_map(root, vaddr, paddr, 0, RW));
  KASSERT(page_table_map(root, VADDR, PADDR, 1, RW));
  KASSERT(!page_table_unmap_range(root, vaddr, 2 * PAGE_SIZE));

  expect_translate(root, vaddr, paddr);
  expect_translate(root, VADDR, PADDR);

  page_table_free(root);
}

void test_page_table() {
  size_t initial_free_count = page_free_count();

  test_map_translate();
  test_aliasing();
  test_map_rejects_invalid_flags();
  test_map_rejects_double_map();
  test_map_rejects_misaligned();
  test_map_rejects_invalid_level();

  test_unmap();
  test_unmap_prune();
  test_unmap_prune_nonempty();
  test_unmap_rejects_unmapped();
  test_unmap_rejects_misaligned();

  test_map_range();
  test_map_range_atomicity();
  test_map_range_reclamation();
  test_map_range_rejects_invalid_size();

  test_unmap_range();
  test_unmap_range_atomicity();
  test_unmap_range_prune();
  test_unmap_range_prune_nonempty();
  test_unmap_range_prune_partial();
  test_unmap_range_rejects_partial_superpage();

  KASSERT(page_free_count() == initial_free_count);
}
