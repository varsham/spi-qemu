#ifndef SPI_WIRE_H
#define SPI_WIRE_H

#include <stddef.h>
#include <stdint.h>

/*
 * Blocking single-byte transfer.
 *
 * Master: sends tx immediately, then blocks until the slave's reply byte
 * arrives. Slave: preloads tx as the byte it will shift out on the next
 * receive, then blocks until the master's byte arrives.
 *
 * Both return the byte received from the other side.
 */
uint8_t spi_wire_master_transfer_byte(uint8_t tx);
uint8_t spi_wire_slave_transfer_byte(uint8_t tx);

/*
 * Multi-byte versions: call the matching *_byte() function once per
 * element of tx/rx, in order. tx or rx may be NULL to ignore that
 * direction (a dummy 0x00 is sent / a received byte is discarded).
 *
 * Known limitation, and one a real SPI slave has too: the slave must
 * reload its next reply byte after finishing one transfer and before the
 * master starts the next one. This loop does that as its first action
 * after each byte, but nothing in the current register set enforces the
 * ordering -- if the master races ahead, the slave may reply with a
 * stale byte. Fine for these POC's back-to-back polling loops; a driver
 * meant for real timing would need a handshake or a double-buffered
 * reply register in the device model.
 */
void spi_wire_master_transfer_buf(const uint8_t *tx, uint8_t *rx, size_t len);
void spi_wire_slave_transfer_buf(const uint8_t *tx, uint8_t *rx, size_t len);

#endif /* SPI_WIRE_H */
