# UART

PnutOS runs on QEMU's RISC-V `virt` machine, which provides a 16550A-compatible UART.

## Configuration

- Base address: `0x10000000`
- UART clock: `3,686,400 Hz`
- Baud rate: `115200`
- Baud divisor: `2`
- Serial format: `8N1`
- RX buffer size: `128 bytes`

## Registers

| Offset | Register | Name | Purpose |
|---|---|---|---|
| `0` | THR | Transmitter Holding Register | Transmit a byte |
| `0` | RBR | Receiver Buffer Register | Receive a byte |
| `0` | DLL | Divisor Latch Low | Set low byte of baud divisor when DLAB = 1 |
| `1` | IER | Interrupt Enable Register | Control UART interrupts |
| `1` | DLM | Divisor Latch High | Set high byte of baud divisor when DLAB = 1 |
| `2` | FCR | FIFO Control Register | Control FIFO buffers |
| `2` | IIR | Interrupt Identification Register | Identify the pending interrupt |
| `3` | LCR | Line Control Register | Configure serial format and DLAB |
| `5` | LSR | Line Status Register | Report line status |

## Receive Path

UART input is handled using interrupts. When a UART receive interrupt is handled, the bytes waiting in the hardware `RX FIFO` are drained into a software ring buffer. `uart_getchar()` reads a character from this buffer.

The ring buffer stores received bytes in FIFO order and has a capacity of 127 bytes. One slot is left unused to distinguish a full buffer from an empty buffer.

## Transmit Path

UART output is handled using polling. `uart_putchar()` waits until the transmit holding register is ready, then writes a character to it.

## Software Interface

```c
void uart_init();
void uart_enable_rx_interrupt();
void uart_putchar(char c);
void uart_puts(const char *s);
char uart_getchar();
```

- `uart_init()` - initializes the UART
- `uart_enable_rx_interrupt()` - enables receive interrupts
- `uart_putchar()` - writes a character
- `uart_puts()` - writes a null-terminated string
- `uart_getchar()` - reads a character
