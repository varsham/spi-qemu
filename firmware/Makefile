CROSS   = riscv64-elf-
CC      = $(CROSS)gcc
CFLAGS  = -march=rv64imac -mabi=lp64 -mcmodel=medany -nostdlib -ffreestanding -O1 -g \
          -Icommon -I../common
LDFLAGS = -T common/linker.ld -nostdlib

COMMON_OBJS = common/start.o common/uart.o common/spi_wire.o

all: master.elf slave.elf

master.elf: $(COMMON_OBJS) master/main.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

slave.elf: $(COMMON_OBJS) slave/main.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f common/*.o master/*.o slave/*.o master.elf slave.elf

.PHONY: all clean
