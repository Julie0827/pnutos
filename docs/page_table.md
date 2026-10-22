# Page Tables

PnutOS uses Sv39, a RISC-V virtual memory mode with a 39-bit virtual address space and three-level page tables.

## Configuration

- Page size: `4096` bytes (4 KiB)
- Page table levels: `3`
- Entries per page table: `512`
- Page table entry size: `8` bytes
- VPN bits per level: `9`
- Page offset bits: `12`

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

```c
page_table_t *page_table_alloc();

bool page_table_map(page_table_t *root, uintptr_t vaddr, uintptr_t paddr, pte_t flags);
bool page_table_map_range(page_table_t *root, uintptr_t vaddr, uintptr_t paddr, size_t size,
                          pte_t flags);

bool page_table_unmap(page_table_t *root, uintptr_t vaddr);
bool page_table_unmap_range(page_table_t *root, uintptr_t vaddr, size_t size);

bool page_table_translate(page_table_t *root, uintptr_t vaddr, uintptr_t *paddr);

void page_table_free(page_table_t *root);
```

- `page_table_alloc()` - allocates a zero-initialized page table
- `page_table_map()` - maps a virtual page to a physical page with the specified flags
- `page_table_map_range()` - maps a range of virtual pages to physical pages
- `page_table_unmap()` - removes a page mapping and frees empty intermediate page tables
- `page_table_unmap_range()` - removes mappings for a range of pages
- `page_table_translate()` - translates a virtual address to a physical address
- `page_table_free()` - frees all page tables in the hierarchy, including the root
