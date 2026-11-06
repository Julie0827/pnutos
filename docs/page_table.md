# Page Tables

PnutOS uses Sv39, a RISC-V virtual memory mode with a 39-bit virtual address space and three-level page tables.

## Configuration

- Page size: `4096` bytes (4 KiB)
- Page table levels: `3`
- Entries per page table: `512`
- Page table entry size: `8` bytes
- VPN bits per level: `9`
- Page offset bits: `12`

### Supported Page Sizes

| Level | Page size | Type |
|---|---|---|
| 0 | 4 KiB | Page |
| 1 | 2 MiB | Superpage |
| 2 | 1 GiB | Superpage |

## Virtual Address

```text
 63       39 38      30 29      21 20      12 11       0
+-----------+----------+----------+----------+----------+
| Sign ext. |  VPN[2]  |  VPN[1]  |  VPN[0]  |  Offset  |
+-----------+----------+----------+----------+----------+
   25 bits    9 bits     9 bits     9 bits     12 bits
```

## Physical Address

```text
 55                       12 11       0
+---------------------------+----------+
|            PPN            |  Offset  |
+---------------------------+----------+
           44 bits             12 bits
```

## Page Table Entry (PTE)

```text
 63      54 53                    10 9   8 7 6 5 4 3 2 1 0
+----------+------------------------+-----+-+-+-+-+-+-+-+-+
| Reserved |          PPN           | RSW |D|A|G|U|X|W|R|V|
+----------+------------------------+-----+-+-+-+-+-+-+-+-+
  10 bits           44 bits          2 bits   1 bit each
```

### PTE Flags

| Bit | Flag | Name |
|---|---|---|
| 0 | V | Valid |
| 1 | R | Readable |
| 2 | W | Writable |
| 3 | X | Executable |
| 4 | U | User-accessible |
| 5 | G | Global |
| 6 | A | Accessed |
| 7 | D | Dirty |

## Software Interface

### Types

```c
typedef uint64_t pte_t;

#define PAGE_TABLE_ENTRY_COUNT (PAGE_SIZE / sizeof(pte_t))

typedef pte_t page_table_t[PAGE_TABLE_ENTRY_COUNT];
```

- `pte_t` - represents a page table entry
- `page_table_t` - represents a page table

### Functions

```c
page_table_t *page_table_alloc();

bool page_table_map(page_table_t *root, uintptr_t vaddr, uintptr_t paddr, size_t level,
                    pte_t flags);
bool page_table_map_range(page_table_t *root, uintptr_t vaddr, uintptr_t paddr, size_t size,
                          size_t level, pte_t flags);

bool page_table_unmap(page_table_t *root, uintptr_t vaddr);
bool page_table_unmap_range(page_table_t *root, uintptr_t vaddr, size_t size);

bool page_table_translate(page_table_t *root, uintptr_t vaddr, uintptr_t *paddr);

void page_table_free(page_table_t *root);
```

- `page_table_alloc()` - allocates a zero-initialized page table
- `page_table_map()` - maps a virtual page to a physical page at the specified level with the specified flags
- `page_table_map_range()` - maps a range of virtual pages to physical pages at the specified level with the specified flags; either maps the entire range or leaves all mappings unchanged
- `page_table_unmap()` - removes a page mapping, automatically determining its level, and frees empty intermediate page tables
- `page_table_unmap_range()` - removes mappings for a range of pages, automatically determining their levels, and frees empty intermediate page tables; either unmaps the entire range or leaves all mappings unchanged
- `page_table_translate()` - translates a virtual address to a physical address, automatically determining the mapping level
- `page_table_free()` - frees all page tables in the hierarchy, including the root, without freeing the mapped physical pages
