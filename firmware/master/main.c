#include <stdint.h>
#include "uart.h"
#include "spi_wire_regs.h"

/* Must match VIRT_SPI_WIRE_BASE in the virt.c patch. */
#define SPI_WIRE_BASE 0x10011000UL
#define SPI_REG(off) (*(volatile uint32_t *)(SPI_WIRE_BASE + (off)))

void main(void)
{
    uart_puts("master: starting\n");

    uint8_t tx = 0x42;
    SPI_REG(SPI_WIRE_REG_TXDATA) = tx;
    SPI_REG(SPI_WIRE_REG_CTRL) = SPI_WIRE_CTRL_START;

    uart_puts("master: sent byte, waiting for reply\n");

    while (!(SPI_REG(SPI_WIRE_REG_STATUS) & SPI_WIRE_STATUS_RX_FULL)) {
    }

    uint32_t rx = SPI_REG(SPI_WIRE_REG_RXDATA);
    if (rx == 0xAA) {
        uart_puts("master: got expected reply 0xAA\n");
    } else {
        uart_puts("master: got UNEXPECTED reply\n");
    }

    for (;;) {
    }
}
