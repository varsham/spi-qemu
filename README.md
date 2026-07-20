# spi-wire: two-QEMU-instance SPI POC

Goal: prove out "two separate QEMU processes exchange bytes over something
that stands in for an SPI wire" before touching any real target instance.

Not real SPI: no modeled clock or chip-select line, whole-byte transfers
only. See `common/spi_wire_regs.h` for the register map and
`qemu-device/spi_wire.c` for the transfer protocol description. See
`ARCHITECTURE.md` for why this uses a chardev-backed socket between two
QEMU processes instead of virtio or QEMU's in-process SSI bus.

## Layout

- `common/spi_wire_regs.h` — register offsets/bits, shared by the QEMU
  device model and both firmware images.
- `qemu-device/` — the QEMU-side device model and the snippets needed to
  wire it into a QEMU source tree (`spi_wire.c`, `spi_wire.h`,
  `Kconfig.snippet`, `meson.build.snippet`, `virt.c.snippet`).
- `firmware/` — bare-metal RISC-V firmware (`master/`, `slave/`, shared
  `common/` boot code, UART driver, and `spi_wire.c`/`spi_wire.h` transfer
  driver), already built and verified to boot standalone (prints over UART,
  then blocks waiting on the SPI device — expected, since stock QEMU
  doesn't have the device yet). `master`/`slave` now exchange a 4-byte
  message via `spi_wire_master_transfer_buf()`/`spi_wire_slave_transfer_buf()`
  instead of a single hardcoded byte, driven by single-byte
  `spi_wire_{master,slave}_transfer_byte()` calls underneath — see
  `firmware/common/spi_wire.h` for the API and a known ordering limitation
  on the slave side.

## 1. Build a QEMU with the spi-wire device

Your installed QEMU (`qemu-system-riscv64 --version` → 11.0.0) is a plain
Homebrew build with no source access, so we build a separate, self-built
QEMU for this POC rather than touching that one.

```sh
git clone --branch v11.0.0 --depth 1 https://gitlab.com/qemu-project/qemu.git ~/qemu-spi-poc
cd ~/qemu-spi-poc
```

Copy the device files in:

```sh
POC=~/Projects/Hardware_Projects/spi-qemu-poc

cp "$POC/qemu-device/spi_wire.c"        hw/misc/spi_wire.c
cp "$POC/qemu-device/spi_wire.h"        include/hw/misc/spi_wire.h
cp "$POC/common/spi_wire_regs.h"        include/hw/misc/spi_wire_regs.h
```

Apply the build-system snippets by hand (they're small and the exact
insertion point matters):

- Append the contents of `qemu-device/Kconfig.snippet` to `hw/misc/Kconfig`.
- Add the one line from `qemu-device/meson.build.snippet` into
  `hw/misc/meson.build`, alongside the other `system_ss.add(when: 'CONFIG_...')`
  entries.
- Follow `qemu-device/virt.c.snippet` to edit `hw/riscv/virt.c`: add the
  include, the `VIRT_SPI_WIRE_BASE` constant, the `create_spi_wire()`
  function, and the call to it from `virt_machine_init()`.
  **Before building**, open `virt_memmap[]` near the top of that file and
  confirm `0x10011000`–`0x10012000` doesn't collide with an existing entry
  in your checked-out version; if it does, pick a free 4K gap instead and
  update `VIRT_SPI_WIRE_BASE` there *and* in
  `firmware/master/main.c`/`firmware/slave/main.c` (the `SPI_WIRE_BASE`
  macro) to match.

Configure and build just the riscv64 target (faster than a full build):

```sh
mkdir -p build && cd build
../configure --target-list=riscv64-softmmu
ninja
```

This will take a while (QEMU is a big project) — expect 10–30+ minutes
depending on your machine. The result you want is
`build/qemu-system-riscv64`.

If `spi_wire.c` fails to compile, the likely cause is a small API mismatch
between what it was written against (QEMU 11.0.0's device/qdev API) and
whatever you actually checked out — the compiler error will point at the
exact line/signature to adjust (this is expected friction when porting a
device written from a documented pattern rather than copied from a
matching source tree).

## 2. Build the firmware (already done, for reference)

```sh
cd ~/Projects/Hardware_Projects/spi-qemu-poc/firmware
make
```

Produces `master.elf` and `slave.elf`. Uses `riscv64-elf-gcc` with
`-mcmodel=medany` (required — the default `medlow` model doesn't work for
code linked at `0x80000000`, since that address sign-extends across the
`medlow` model's assumed low/high-2GB split and the linker will fail with
`relocation truncated to fit`).

## 3. Run both instances

Use the QEMU binary you just built (`~/qemu-spi-poc/build/qemu-system-riscv64`),
**not** the Homebrew one — the Homebrew build doesn't have the spi-wire
device.

Open two terminals.

**Terminal 1 — master (socket server):**

```sh
QEMU=~/qemu-spi-poc/build/qemu-system-riscv64
FW=~/Projects/Hardware_Projects/spi-qemu-poc/firmware

rm -f /tmp/spi_wire.sock
$QEMU -M virt -bios none -nographic \
  -kernel "$FW/master.elf" \
  -chardev socket,id=spiwire0,path=/tmp/spi_wire.sock,server=on,wait=off \
  -global spi-wire.chardev=spiwire0 \
  -global spi-wire.role-master=1
```

**Terminal 2 — slave (socket client), started right after terminal 1:**

```sh
QEMU=~/qemu-spi-poc/build/qemu-system-riscv64
FW=~/Projects/Hardware_Projects/spi-qemu-poc/firmware

$QEMU -M virt -bios none -nographic \
  -kernel "$FW/slave.elf" \
  -chardev socket,id=spiwire0,path=/tmp/spi_wire.sock,server=off \
  -global spi-wire.chardev=spiwire0 \
  -global spi-wire.role-master=0
```

Order matters: the server side must be up (its socket bound) before the
client side tries to connect. If you start the slave first, its chardev
connect will fail immediately since nothing is listening on that path yet.

### Expected output

Terminal 1 (master):
```
master: starting
master: transfer done, checking reply
master: got expected 4-byte reply
```

Terminal 2 (slave):
```
slave: starting
slave: transfer done, checking bytes from master
slave: got expected 4-byte sequence from master
```

Both then spin forever (`for (;;) {}`) — use Ctrl-A X in each terminal to
quit QEMU (standard `-nographic` escape sequence), or kill the processes.

## Next steps (per the staged plan)

Once this works end to end, swap `master.elf`/its QEMU instance for your
specific instance first, keeping the bare-bones slave running, to confirm
the wire protocol still holds with one real side. Then do the same for the
other side. Whether that's "recompile the specific instance with
spi-wire added" or "adapt spi-wire's protocol to something the specific
instance already exposes" depends on whether you have source access to it
— worth confirming before that step.
