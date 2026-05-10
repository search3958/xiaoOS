BUILD := build

BIOS_CC := x86_64-elf-gcc
BIOS_OBJCOPY := x86_64-elf-objcopy
CLANG := clang
LLD_LINK := lld-link
NASM := nasm
STAGE2_SECTORS := 32
STAGE2_SIZE := 16384

BIOS_APP_OBJS :=
UEFI_APP_OBJS :=

ifneq ($(wildcard apps/hello.c),)
BIOS_APP_OBJS += $(BUILD)/bios/hello.o
UEFI_APP_OBJS += $(BUILD)/uefi/hello.obj
endif

ifneq ($(wildcard apps/serial_hello.c),)
BIOS_APP_OBJS += $(BUILD)/bios/serial_hello.o
UEFI_APP_OBJS += $(BUILD)/uefi/serial_hello.obj
endif

.PHONY: all pc bios uefi arm64 arduino esp32c3 esp32c3-upload esp32c3-monitor check clean

all: pc

pc: bios uefi

check: pc arm64

bios: $(BUILD)/bios/xiao-bios.img

uefi: $(BUILD)/uefi/BOOTX64.EFI $(BUILD)/uefi/esp/EFI/BOOT/BOOTX64.EFI

arm64: $(BUILD)/arm64/BOOTAA64.EFI $(BUILD)/arm64/esp/EFI/BOOT/BOOTAA64.EFI

$(BUILD)/bios $(BUILD)/uefi $(BUILD)/uefi/esp/EFI/BOOT $(BUILD)/arm64 $(BUILD)/arm64/esp/EFI/BOOT:
	mkdir -p $@

$(BUILD)/bios/boot.bin: hal/bios/boot.asm | $(BUILD)/bios
	$(NASM) -f bin -DSTAGE2_SECTORS=$(STAGE2_SECTORS) $< -o $@

$(BUILD)/bios/start32.o: hal/bios/start32.asm | $(BUILD)/bios
	$(NASM) -f elf32 $< -o $@

$(BUILD)/bios/xiao_core.o: kernel/core/xiao_core.c include/xiao.h | $(BUILD)/bios
	$(BIOS_CC) -m32 -Iinclude -ffreestanding -fno-stack-protector -fno-pic -fno-pie -mno-sse -mno-mmx -Os -Wall -Wextra -c $< -o $@

$(BUILD)/bios/hello.o: apps/hello.c include/xiao.h | $(BUILD)/bios
	$(BIOS_CC) -m32 -Iinclude -ffreestanding -fno-stack-protector -fno-pic -fno-pie -mno-sse -mno-mmx -Os -Wall -Wextra -c $< -o $@

$(BUILD)/bios/serial_hello.o: apps/serial_hello.c include/xiao.h | $(BUILD)/bios
	$(BIOS_CC) -m32 -Iinclude -ffreestanding -fno-stack-protector -fno-pic -fno-pie -mno-sse -mno-mmx -Os -Wall -Wextra -c $< -o $@

$(BUILD)/bios/image.o: boot/common/image.c include/xiao.h | $(BUILD)/bios
	$(BIOS_CC) -m32 -Iinclude -ffreestanding -fno-stack-protector -fno-pic -fno-pie -mno-sse -mno-mmx -Os -Wall -Wextra -c $< -o $@

$(BUILD)/bios/app_stubs.o: boot/common/app_stubs.c include/xiao.h | $(BUILD)/bios
	$(BIOS_CC) -m32 -Iinclude -ffreestanding -fno-stack-protector -fno-pic -fno-pie -mno-sse -mno-mmx -Os -Wall -Wextra -c $< -o $@

$(BUILD)/bios/bios.o: hal/bios/bios.c include/xiao.h | $(BUILD)/bios
	$(BIOS_CC) -m32 -Iinclude -ffreestanding -fno-stack-protector -fno-pic -fno-pie -mno-sse -mno-mmx -Os -Wall -Wextra -c $< -o $@

$(BUILD)/bios/stage2.elf: $(BUILD)/bios/start32.o $(BUILD)/bios/xiao_core.o $(BUILD)/bios/app_stubs.o $(BIOS_APP_OBJS) $(BUILD)/bios/image.o $(BUILD)/bios/bios.o hal/bios/linker.ld
	$(BIOS_CC) -m32 -nostdlib -Wl,-m,elf_i386 -Wl,--build-id=none -T hal/bios/linker.ld $(filter %.o,$^) -o $@

$(BUILD)/bios/stage2.raw: $(BUILD)/bios/stage2.elf
	$(BIOS_OBJCOPY) -O binary $< $@

$(BUILD)/bios/stage2.bin: $(BUILD)/bios/stage2.raw
	test $$(wc -c < $<) -le $(STAGE2_SIZE)
	dd if=/dev/zero of=$@ bs=$(STAGE2_SIZE) count=1 2>/dev/null
	dd if=$< of=$@ conv=notrunc 2>/dev/null

$(BUILD)/bios/xiao-bios.img: $(BUILD)/bios/boot.bin $(BUILD)/bios/stage2.bin
	cat $^ > $@

UEFI_CFLAGS := -target x86_64-pc-win32 -DXIAO_UEFI_X86_SERIAL -Iinclude -Ihal/uefi -ffreestanding -fshort-wchar -fno-stack-protector -mno-red-zone -Os -Wall -Wextra
ARM64_UEFI_CFLAGS := -target aarch64-unknown-windows -Iinclude -Ihal/uefi -ffreestanding -fshort-wchar -fno-stack-protector -Os -Wall -Wextra

$(BUILD)/uefi/xiao_core.obj: kernel/core/xiao_core.c include/xiao.h | $(BUILD)/uefi
	$(CLANG) $(UEFI_CFLAGS) -c $< -o $@

$(BUILD)/uefi/hello.obj: apps/hello.c include/xiao.h | $(BUILD)/uefi
	$(CLANG) $(UEFI_CFLAGS) -c $< -o $@

$(BUILD)/uefi/serial_hello.obj: apps/serial_hello.c include/xiao.h | $(BUILD)/uefi
	$(CLANG) $(UEFI_CFLAGS) -c $< -o $@

$(BUILD)/uefi/image.obj: boot/common/image.c include/xiao.h | $(BUILD)/uefi
	$(CLANG) $(UEFI_CFLAGS) -c $< -o $@

$(BUILD)/uefi/app_stubs.obj: boot/common/app_stubs.c include/xiao.h | $(BUILD)/uefi
	$(CLANG) $(UEFI_CFLAGS) -c $< -o $@

$(BUILD)/uefi/uefi.obj: hal/uefi/uefi.c hal/uefi/efi.h include/xiao.h | $(BUILD)/uefi
	$(CLANG) $(UEFI_CFLAGS) -c $< -o $@

$(BUILD)/uefi/portio.obj: hal/uefi/portio.asm | $(BUILD)/uefi
	$(NASM) -f win64 $< -o $@

$(BUILD)/uefi/BOOTX64.EFI: $(BUILD)/uefi/xiao_core.obj $(BUILD)/uefi/app_stubs.obj $(UEFI_APP_OBJS) $(BUILD)/uefi/image.obj $(BUILD)/uefi/uefi.obj $(BUILD)/uefi/portio.obj
	$(LLD_LINK) /subsystem:efi_application /entry:efi_main /nodefaultlib /out:$@ $^

$(BUILD)/uefi/esp/EFI/BOOT/BOOTX64.EFI: $(BUILD)/uefi/BOOTX64.EFI | $(BUILD)/uefi/esp/EFI/BOOT
	cp $< $@

$(BUILD)/arm64/xiao_core.obj: kernel/core/xiao_core.c include/xiao.h | $(BUILD)/arm64
	$(CLANG) $(ARM64_UEFI_CFLAGS) -c $< -o $@

$(BUILD)/arm64/hello.obj: apps/hello.c include/xiao.h | $(BUILD)/arm64
	$(CLANG) $(ARM64_UEFI_CFLAGS) -c $< -o $@

$(BUILD)/arm64/serial_hello.obj: apps/serial_hello.c include/xiao.h | $(BUILD)/arm64
	$(CLANG) $(ARM64_UEFI_CFLAGS) -c $< -o $@

$(BUILD)/arm64/image.obj: boot/common/image.c include/xiao.h | $(BUILD)/arm64
	$(CLANG) $(ARM64_UEFI_CFLAGS) -c $< -o $@

$(BUILD)/arm64/app_stubs.obj: boot/common/app_stubs.c include/xiao.h | $(BUILD)/arm64
	$(CLANG) $(ARM64_UEFI_CFLAGS) -c $< -o $@

$(BUILD)/arm64/uefi.obj: hal/uefi/uefi.c hal/uefi/efi.h include/xiao.h | $(BUILD)/arm64
	$(CLANG) $(ARM64_UEFI_CFLAGS) -c $< -o $@

$(BUILD)/arm64/BOOTAA64.EFI: $(BUILD)/arm64/xiao_core.obj $(BUILD)/arm64/app_stubs.obj $(UEFI_APP_OBJS:$(BUILD)/uefi/%=$(BUILD)/arm64/%) $(BUILD)/arm64/image.obj $(BUILD)/arm64/uefi.obj
	$(LLD_LINK) /machine:arm64 /subsystem:efi_application /entry:efi_main /nodefaultlib /out:$@ $^

$(BUILD)/arm64/esp/EFI/BOOT/BOOTAA64.EFI: $(BUILD)/arm64/BOOTAA64.EFI | $(BUILD)/arm64/esp/EFI/BOOT
	cp $< $@

arduino:
	@command -v arduino-cli >/dev/null || { echo "arduino-cli is required for this target"; exit 127; }
	arduino-cli compile --fqbn arduino:avr:uno hal/arduino/xiaoOS

esp32c3:
	./esp32c3.sh compile

esp32c3-upload:
	./esp32c3.sh upload

esp32c3-monitor:
	./esp32c3.sh monitor

clean:
	rm -rf $(BUILD)
