BUILD := build

CC      := gcc
AS      := nasm
LD      := ld
OBJCOPY := objcopy
QEMU    := qemu-system-i386

CFLAGS := \
	-m32 \
	-ffreestanding \
	-fno-pie \
	-fno-stack-protector \
	-mno-sse \
	-mno-sse2 \
	-fno-builtin \
	-O0

.PHONY: all run clean rebuild

images:
	./pngtovga.py ./无信号.jpg src/nosignalimg.h nosignal

all: images smallos32.img


# Bootloader

$(BUILD)/boot.bin: boot.asm
	@mkdir -p $(dir $@)
	@echo "  NASM    $<"
	@$(AS) -f bin $< -o $@


# Kernel C

$(BUILD)/kernel.o: src/kernel.c
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) -c $< -o $@


# Link

$(BUILD)/kernel.elf: $(BUILD)/kernel.o $(BUILD/syscalls.o) linkity.ld
	@mkdir -p $(dir $@)
	@echo "  LD      $@"
	@$(LD) -m elf_i386 -T linkity.ld -o $@ $(BUILD)/kernel.o


# Raw kernel binary

$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	@echo "  OBJCOPY $@"
	@$(OBJCOPY) -O binary $< $@


# Pad kernel to 32 sectors

$(BUILD)/kernel.pad: $(BUILD)/kernel.bin
	@echo "  PAD     $@"
	@dd if=/dev/zero of=$@ bs=512 count=32 status=none
	@dd if=$< of=$@ conv=notrunc status=none


# Disk image

smallos32.img: $(BUILD)/boot.bin $(BUILD)/kernel.pad
	@echo "  IMAGE   $@"
	@cat $^ > $@
	@dd if=smallos32.img of=disk.img bs=512 conv=notrunc


# Run

run: disk.img
	$(QEMU) -drive format=raw,file=$<


# Cleaning

clean:
	rm -rf $(BUILD) smallos32.img

rebuild: clean all
