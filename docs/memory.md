# Memory

The RAM base address and size are discovered from the DTB during boot, and the kernel and stack ranges are provided by linker symbols.

## Memory Information

- Kernel memory range
- Stack memory range
- RAM range
- RAM size

## Software Interface

```c
struct memory_size {
  uint64_t val;
  const char *unit;
};

void memory_init(uint64_t base, uint64_t size);
uint64_t memory_get_end();
struct memory_size memory_get_formatted_size();
void memory_print_info(const char *prefix);
```

- `struct memory_size` - stores the formatted RAM size and its unit
- `memory_init()` - initializes the RAM information
- `memory_get_end()` - returns the end address of RAM
- `memory_get_formatted_size()` - returns the RAM size in MiB or GiB
- `memory_print_info()` - prints the kernel, stack, and RAM information

## Output

Example:

```text
kernel: 0x80000000 - 0x800015c0
stack: 0x800015c0 - 0x800055c0
ram: 0x80000000 - 0x88000000 (128 MiB)
```
