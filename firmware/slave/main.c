#include <stdint.h>
#include "uart.h"
#include "spi_wire_regs.h"

/* Must match VIRT_SPI_WIRE_BASE in the virt.c patch. */
#define SPI_WIRE_BASE 0x10011000UL
#define SPI_REG(off) (*(volatile uint32_t *)(SPI_WIRE_BASE + (off)))

void main(void)
{
    uart_puts("slave: starting, pre-loading reply byte 0xAA\n");

    /* Slave never initiates -- it just makes sure TXDATA holds whatever
     * it wants to shift out the next time the master starts a transfer. */
    SPI_REG(SPI_WIRE_REG_TXDATA) = 0xAA;

    while (!(SPI_REG(SPI_WIRE_REG_STATUS) & SPI_WIRE_STATUS_RX_FULL)) {
    }

    uint32_t rx = SPI_REG(SPI_WIRE_REG_RXDATA);
    if (rx == 0x42) {
        uart_puts("slave: got expected byte 0x42 from master, replied 0xAA\n");
    } else {
        uart_puts("slave: got UNEXPECTED byte from master\n");
    }

    for (;;) {
    }
}
