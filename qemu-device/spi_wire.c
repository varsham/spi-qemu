/*
 * spi-wire: a minimal cross-process "SPI over a byte stream" POC device.
 *
 * Copy to: <qemu-src>/hw/misc/spi_wire.c
 * Also copy: common/spi_wire_regs.h -> <qemu-src>/include/hw/misc/spi_wire_regs.h
 *            qemu-device/spi_wire.h -> <qemu-src>/include/hw/misc/spi_wire.h
 *
 * This is not a real SPI controller: there's no modeled clock or chip-select
 * line, and every transfer is exactly one byte. The "wire" is a chardev
 * (intended backend: a UNIX domain socket), so two independent QEMU
 * processes can each instantiate this device, point their chardevs at the
 * same socket path (one server, one client), and exchange bytes.
 *
 * Transfer protocol:
 *   - Master role: guest writes TXDATA, then writes CTRL.START. The device
 *     sends that one byte out over the chardev immediately.
 *   - Slave role: guest pre-loads TXDATA with whatever it wants to reply
 *     with on the *next* transfer. The device is otherwise passive.
 *   - On chardev receive (either role): the received byte lands in RXDATA
 *     and STATUS.RX_FULL is set. If this instance is the slave, it
 *     immediately shifts out its current TXDATA as the reply -- mirroring
 *     how a real SPI slave has no say over when a transfer happens.
 */

#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "hw/misc/spi_wire.h"
#include "hw/misc/spi_wire_regs.h"
#include "chardev/char-fe.h"
#include "qapi/error.h"
#include "qom/object.h"

OBJECT_DECLARE_SIMPLE_TYPE(SPIWireState, SPI_WIRE)

struct SPIWireState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;
    CharBackend chr;

    uint8_t role_is_master;
    uint32_t txdata;
    uint32_t rxdata;
    uint32_t status;
};

static uint64_t spi_wire_read(void *opaque, hwaddr addr, unsigned size)
{
    SPIWireState *s = SPI_WIRE(opaque);

    switch (addr) {
    case SPI_WIRE_REG_TXDATA:
        return s->txdata;
    case SPI_WIRE_REG_RXDATA:
        s->status &= ~SPI_WIRE_STATUS_RX_FULL;
        return s->rxdata;
    case SPI_WIRE_REG_STATUS:
        return s->status;
    default:
        return 0;
    }
}

static void spi_wire_write(void *opaque, hwaddr addr, uint64_t val,
                            unsigned size)
{
    SPIWireState *s = SPI_WIRE(opaque);
    uint8_t byte;

    switch (addr) {
    case SPI_WIRE_REG_TXDATA:
        s->txdata = val & 0xff;
        break;
    case SPI_WIRE_REG_CTRL:
        if ((val & SPI_WIRE_CTRL_START) && s->role_is_master) {
            byte = s->txdata & 0xff;
            qemu_chr_fe_write_all(&s->chr, &byte, 1);
        }
        break;
    default:
        break;
    }
}

static const MemoryRegionOps spi_wire_ops = {
    .read = spi_wire_read,
    .write = spi_wire_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static int spi_wire_chr_can_receive(void *opaque)
{
    return 1;
}

static void spi_wire_chr_receive(void *opaque, const uint8_t *buf, int size)
{
    SPIWireState *s = SPI_WIRE(opaque);
    uint8_t reply;

    s->rxdata = buf[0];
    s->status |= SPI_WIRE_STATUS_RX_FULL;

    if (!s->role_is_master) {
        reply = s->txdata & 0xff;
        qemu_chr_fe_write_all(&s->chr, &reply, 1);
    }
}

static void spi_wire_chr_event(void *opaque, QEMUChrEvent event)
{
    /* Connect/disconnect events aren't modeled for this POC. */
}

static void spi_wire_realize(DeviceState *dev, Error **errp)
{
    SPIWireState *s = SPI_WIRE(dev);

    qemu_chr_fe_set_handlers(&s->chr, spi_wire_chr_can_receive,
                              spi_wire_chr_receive, spi_wire_chr_event,
                              NULL, s, NULL, true);
}

static Property spi_wire_properties[] = {
    DEFINE_PROP_CHR("chardev", SPIWireState, chr),
    DEFINE_PROP_UINT8("role-master", SPIWireState, role_is_master, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static void spi_wire_init(Object *obj)
{
    SPIWireState *s = SPI_WIRE(obj);

    memory_region_init_io(&s->mmio, obj, &spi_wire_ops, s,
                           TYPE_SPI_WIRE, SPI_WIRE_MMIO_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static void spi_wire_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = spi_wire_realize;
    device_class_set_props(dc, spi_wire_properties);
}

static const TypeInfo spi_wire_info = {
    .name = TYPE_SPI_WIRE,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(SPIWireState),
    .instance_init = spi_wire_init,
    .class_init = spi_wire_class_init,
};

static void spi_wire_register_types(void)
{
    type_register_static(&spi_wire_info);
}

type_init(spi_wire_register_types)
