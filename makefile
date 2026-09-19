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
	-O0 \
	-Isrc

CFILES := $(shell find src -name '*.c')
AFILES := $(shell find src -name '*.asm')

COBJS := $(CFILES:src/%.c=$(BUILD)/%.o)
AOBJS := $(AFILES:src/%.asm=$(BUILD)/%.o)

OBJS := $(COBJS) $(AOBJS)

.PHONY: all run clean rebuild images

all: images smallos32.img

images:
	./pngtovga.py --all-images-in-pwd -out=src/kernel/images.h -bpp=2


# Bootloader

$(BUILD)/boot.bin: boot.asm
	@mkdir -p $(dir $@)
	@echo "  NASM    $<"
	@$(AS) -f bin $< -o $@


# C source

$(BUILD)/%.o: src/%.c
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) -c $< -o $@


# Assembly source

$(BUILD)/%.o: src/%.asm
	@mkdir -p $(dir $@)
	@echo "  NASM    $<"
	@$(AS) -f elf32 $< -o $@


# Link

$(BUILD)/kernel.elf: $(OBJS) linkity.ld
	@mkdir -p $(dir $@)
	@echo "  LD      $@"
	@$(LD) -m elf_i386 -T linkity.ld -o $@ $(OBJS)


# Raw kernel binary

$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	@echo "  OBJCOPY $@"
	@$(OBJCOPY) -O binary $< $@


# Pad kernel to 32 sectors

$(BUILD)/kernel.pad: $(BUILD)/kernel.bin
	@dd if=/dev/zero of=$@ bs=512 count=64 status=none
	@dd if=$< of=$@ conv=notrunc status=none


# Disk image

smallos32.img: $(BUILD)/boot.bin $(BUILD)/kernel.pad
	@echo "  IMAGE   $@"
	@cat $^ > $@
	@dd if=smallos32.img of=disk.img bs=512 conv=notrunc status=none


# Run

run: disk.img
	$(QEMU) -drive format=raw,file=$<


# Cleaning

clean:
	rm -rf $(BUILD) smallos32.img disk.img

rebuild: clean all
