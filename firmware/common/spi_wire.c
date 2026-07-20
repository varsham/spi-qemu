#include <stddef.h>
#include <stdint.h>
#include "spi_wire.h"
#include "spi_wire_regs.h"

/* Must match VIRT_SPI_WIRE_BASE in the virt.c patch. */
#define SPI_WIRE_BASE 0x10011000UL
#define SPI_REG(off) (*(volatile uint32_t *)(SPI_WIRE_BASE + (off)))

static uint8_t spi_wire_wait_and_read(void)
{
    while (!(SPI_REG(SPI_WIRE_REG_STATUS) & SPI_WIRE_STATUS_RX_FULL)) {
    }
    return (uint8_t)SPI_REG(SPI_WIRE_REG_RXDATA);
}

uint8_t spi_wire_master_transfer_byte(uint8_t tx)
{
    SPI_REG(SPI_WIRE_REG_TXDATA) = tx;
    SPI_REG(SPI_WIRE_REG_CTRL) = SPI_WIRE_CTRL_START;
    return spi_wire_wait_and_read();
}

uint8_t spi_wire_slave_transfer_byte(uint8_t tx)
{
    SPI_REG(SPI_WIRE_REG_TXDATA) = tx;
    return spi_wire_wait_and_read();
}

void spi_wire_master_transfer_buf(const uint8_t *tx, uint8_t *rx, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        uint8_t r = spi_wire_master_transfer_byte(tx ? tx[i] : 0x00);
        if (rx) {
            rx[i] = r;
        }
    }
}

void spi_wire_slave_transfer_buf(const uint8_t *tx, uint8_t *rx, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        uint8_t r = spi_wire_slave_transfer_byte(tx ? tx[i] : 0x00);
        if (rx) {
            rx[i] = r;
        }
    }
}
