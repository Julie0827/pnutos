# 🥜 PnutOS

## Tech Stack

- C23
- RISC-V (RV64GC) assembly
- QEMU (`virt` machine)
- RISC-V GNU toolchain (`riscv64-unknown-elf-gcc`)

## Setup

I'm using macOS, so these setup instructions are written for macOS.

### QEMU

PnutOS uses QEMU to emulate a RISC-V computer. If you don't have it already, you can install it with Homebrew:

```bash
brew install qemu
```

You can check that it installed correctly using:

```bash
qemu-system-riscv64 --version
```

### RISC-V Toolchain

Building PnutOS requires the `riscv64-unknown-elf-gcc` cross compiler (part of the RISC-V GNU toolchain). If you don't have it already, you can install it with Homebrew:

```bash
brew install riscv-software-src/riscv/riscv-gnu-toolchain
```

You can check that it installed correctly using:

```bash
riscv64-unknown-elf-gcc -dumpmachine
```

You should see:

```text
riscv64-unknown-elf
```

> [!NOTE]
> PnutOS is written in C23, so the cross compiler must support C23.

## Build and Run

You can use the `Makefile` to build and run PnutOS.

To build PnutOS, run:

```bash
make
```

To build PnutOS and run it in QEMU, run:

```bash
make run
```

QEMU is set to run directly in the terminal instead of opening a separate window.

> [!NOTE]
> To exit QEMU, press <kbd>Ctrl</kbd> + <kbd>A</kbd>, release the keys, then press <kbd>X</kbd>.

PnutOS runs with 128 MiB of RAM by default. You can change the RAM size using `MEM` (`M` for MiB or `G` for GiB).

For example:

```bash
make run MEM=512M
make run MEM=2G
```

To build PnutOS and run the test suite in QEMU, run:

```bash
make test
```

To test with a different RAM size, you can use `MEM` just like with `make run`.

For example:

```bash
make test MEM=2G
```

To remove all generated build files, run:

```bash
make clean
```
