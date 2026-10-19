#include "uart.h"

#include <stddef.h>

#define UART_BASE 0x10000000UL

#define UART_THR 0
#define UART_RBR 0
#define UART_DLL 0

#define UART_IER 1
#define UART_DLM 1

#define UART_FCR 2
#define UART_IIR 2

#define UART_LCR 3
#define UART_LSR 5

#define UART_IER_RDI 0x01

#define UART_FCR_ENABLE 0x01
#define UART_FCR_CLEAR_RX 0x02
#define UART_FCR_CLEAR_TX 0x04

#define UART_IIR_NO_INTERRUPT 0x01
#define UART_IIR_ID_MASK 0x0E
#define UART_IIR_RDI 0x04
#define UART_IIR_TIMEOUT 0x0C

#define UART_LCR_DLAB (1U << 7)
#define UART_LCR_8N1 0x03

#define UART_LSR_THRE (1U << 5)
#define UART_LSR_DR 0x01

#define UART_REG(offset) (*(volatile unsigned char *)(UART_BASE + (offset)))

#define UART_RX_BUFFER_SIZE 128

static char rx_buffer[UART_RX_BUFFER_SIZE];
static size_t rx_read;
static size_t rx_write;

void uart_init() {
  UART_REG(UART_IER) = 0;

  UART_REG(UART_LCR) = UART_LCR_DLAB;

  UART_REG(UART_DLL) = 2;
  UART_REG(UART_DLM) = 0;

  UART_REG(UART_LCR) = UART_LCR_8N1;

  UART_REG(UART_FCR) = UART_FCR_ENABLE | UART_FCR_CLEAR_RX | UART_FCR_CLEAR_TX;
}

void uart_enable_rx_interrupt() {
  UART_REG(UART_IER) |= UART_IER_RDI;
}

static bool rx_buffer_write(char c) {
  size_t next = (rx_write + 1) % UART_RX_BUFFER_SIZE;

  if (next == rx_read) return false;

  rx_buffer[rx_write] = c;
  rx_write = next;

  return true;
}

static bool rx_buffer_read(char *c) {
  if (rx_read == rx_write) return false;

  *c = rx_buffer[rx_read];
  rx_read = (rx_read + 1) % UART_RX_BUFFER_SIZE;

  return true;
}

static void uart_drain_rx() {
  while (UART_REG(UART_LSR) & UART_LSR_DR) {
    rx_buffer_write(UART_REG(UART_RBR));
  }
}

void uart_handle_interrupt() {
  unsigned char iir = UART_REG(UART_IIR);

  if (iir & UART_IIR_NO_INTERRUPT) return;

  unsigned char id = iir & UART_IIR_ID_MASK;

  switch (id) {
    case UART_IIR_RDI:
    case UART_IIR_TIMEOUT:
      uart_drain_rx();
      break;
  }
}

void uart_putchar(char c) {
  while ((UART_REG(UART_LSR) & UART_LSR_THRE) == 0) {
  }

  UART_REG(UART_THR) = c;
}

void uart_puts(const char *s) {
  while (*s) {
    uart_putchar(*s++);
  }
}

char uart_getchar() {
  char c;

  while (!rx_buffer_read(&c)) {
  }

  return c;
}
