#ifndef SPI_WIRE_REGS_H
#define SPI_WIRE_REGS_H

/*
 * Register map for the "spi-wire" POC peripheral.
 * Shared verbatim between the QEMU device model (host build) and the
 * bare-metal guest driver (target build) so the two never drift apart.
 *
 * Not a real SPI controller: there is no clock or chip-select line, and
 * every transfer is one whole byte. It exists to prove out the "two
 * separate QEMU processes exchange bytes over a wire" mechanism before
 * building anything closer to real SPI timing.
 */

#define SPI_WIRE_REG_TXDATA   0x00u  /* write: byte to send (master) / byte to shift out next (slave) */
#define SPI_WIRE_REG_RXDATA   0x04u  /* read: last byte received; read clears STATUS.RX_FULL */
#define SPI_WIRE_REG_STATUS   0x08u  /* read: SPI_WIRE_STATUS_* bits */
#define SPI_WIRE_REG_CTRL     0x0Cu  /* write: SPI_WIRE_CTRL_* bits */

#define SPI_WIRE_STATUS_RX_FULL   (1u << 0)  /* a byte has arrived and hasn't been read yet */

#define SPI_WIRE_CTRL_START       (1u << 0)  /* master only: send TXDATA now */

#define SPI_WIRE_MMIO_SIZE 0x1000u

#endif /* SPI_WIRE_REGS_H */
