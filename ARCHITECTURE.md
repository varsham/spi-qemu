# Why chardev, not virtio or QEMU's SSI bus

This came up as a point of confusion while building the POC, so it's worth
writing down properly: "chardev", "virt", "virtio", and QEMU's SSI bus all
sound like they might be competing choices for how to model SPI. They're
not. They sit at different layers, and only one of them is actually a
transport option for this project's topology. This doc explains why.

## The three things being conflated

**`-M virt`** is a *machine type* — the RISC-V board model (memory map,
UART, CLINT/PLIC, and now the spi-wire device) that a single firmware image
boots on. It answers "what hardware does the guest see." It has nothing to
do with how bytes move between two separate QEMU processes; that question
doesn't exist yet at this layer. `qemu-device/virt.c.snippet` patches this
board's memory map either way, regardless of which transport is chosen
below.

**`chardev`** is QEMU's generic byte-stream backend abstraction — a
"bytes in, bytes out" endpoint that can be backed by a UNIX socket, a TCP
socket, a pty, stdio, a file, etc. It's not specific to serial ports and
it's not legacy; it's the standard way to connect a device *model*'s data
to something outside the device model. `spi_wire.c` uses it via
`CharBackend`/`qemu_chr_fe_write_all()` (see `qemu-device/spi_wire.c`,
`spi_wire_write()` and `spi_wire_chr_receive()`).

**Virtio** is a paravirtualization framework: virtqueues, descriptor
rings, feature negotiation. It's a protocol between a guest driver that
*knows* it's talking to a hypervisor and a device model that only exists
inside one. There is no real chip on a physical board that speaks virtio —
it has no hardware equivalent, by design.

**QEMU's SSI bus** (`include/hw/ssi/ssi.h`, `hw/ssi/ssi.c`) is QEMU's
actual in-process model of a real SPI bus. A controller device and a
peripheral device (e.g. an SPI flash chip) are both instantiated in the
*same* QEMU process, wired together via `SSIBus`, and the controller calls

```c
uint32_t ssi_transfer(SSIBus *bus, uint32_t val);
```

directly against the peripheral — a plain C function call, no bytes leave
the process. This is genuinely how QEMU models e.g. an MCU's SPI
controller wired to an SPI flash on the same simulated board.

## Why this project can't use the SSI bus

`ssi_transfer()` is a function call within one address space. It requires
both sides of the transfer to be objects inside the same QEMU process.

This project's whole premise (see the top of `README.md`) is the opposite:
**two independent QEMU processes**, each standing in for a separate real
chip/board, with the explicit intent to swap one side at a time for actual
hardware later while the other keeps running in QEMU. Two independent
processes have no shared memory and no way to call into each other
directly — `ssi_transfer()` cannot cross that boundary, because on real
hardware there *is* no shared address space between two separate chips
either. The only thing connecting them, on real hardware or in this
simulation, is the wire itself.

A socket, via `chardev`, is QEMU's way of putting an actual OS-level
channel underneath a device model. That's not a workaround standing in for
"real" SPI modeling — for this specific topology (two separate targets),
it's the only mechanism that can play the role of the physical wire at
all.

## Comparison

| Mechanism | Where it lives | What it models | Applies here? |
|---|---|---|---|
| SSI bus (`ssi_transfer()`) | In-process, same QEMU | Controller + peripheral on *one* board | No — needs both sides in one process |
| virtio | In-process, guest+hypervisor | Paravirtualized device with no hardware equivalent | No — goal is real-hardware-shaped register I/O |
| `chardev` over a UNIX socket | Cross-process | The physical wire between two separate chips/boards | **Yes** — this project's actual topology |
| `-M virt` | N/A (it's the board, not a transport) | The memory map each firmware image boots on | Used regardless of the above |

## What would change the answer

If the real target were instead a single chip talking to an on-board SPI
peripheral (e.g. one MCU wired to one SPI flash, both meant to be modeled
in one simulated system), the SSI bus would be the right tool and this
project's two-process/chardev design would be the wrong starting point —
that would call for one QEMU process with a controller device and a
peripheral device joined via `ssi_create_bus()`/`ssi_transfer()`, no
sockets involved. That is not this project's goal: the two sides here are
meant to become two independently real, physically separate targets, which
is exactly the case `chardev` over a socket is for.
