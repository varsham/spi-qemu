#include <stdint.h>
#include "uart.h"
#include "spi_wire.h"

void main(void)
{
    uart_puts("master: starting\n");

    static const uint8_t tx[4] = { 0x01, 0x02, 0x03, 0x04 };
    static const uint8_t expected_rx[4] = { 0xAA, 0xBB, 0xCC, 0xDD };
    uint8_t rx[4];

    spi_wire_master_transfer_buf(tx, rx, sizeof(tx));

    uart_puts("master: transfer done, checking reply\n");

    int ok = 1;
    for (unsigned i = 0; i < sizeof(rx); i++) {
        if (rx[i] != expected_rx[i]) {
            ok = 0;
        }
    }

    if (ok) {
        uart_puts("master: got expected 4-byte reply\n");
    } else {
        uart_puts("master: got UNEXPECTED reply\n");
    }

    for (;;) {
    }
}
