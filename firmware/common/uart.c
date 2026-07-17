#include <stdint.h>
#include "uart.h"

/* virt machine's UART0: 16550-compatible, MMIO at 0x10000000. */
#define UART0_BASE      0x10000000UL
#define UART_REG(off)   (*(volatile uint8_t *)(UART0_BASE + (off)))
#define UART_THR        UART_REG(0)
#define UART_LSR        UART_REG(5)
#define UART_LSR_THRE   (1u << 5)

void uart_putc(char c)
{
    while (!(UART_LSR & UART_LSR_THRE)) {
    }
    UART_THR = (uint8_t)c;
}

void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}
