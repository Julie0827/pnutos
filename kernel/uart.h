#pragma once

void uart_init();
void uart_enable_rx_interrupt();
void uart_handle_interrupt();

void uart_putchar(char c);
void uart_puts(const char *s);
char uart_getchar();
