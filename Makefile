BUILD := build
PYTHON := python3

BIOS_CC := x86_64-elf-gcc
BIOS_OBJCOPY := x86_64-elf-objcopy
CLANG := clang
LLD_LINK := lld-link
NASM := nasm
STAGE2_SECTORS := 128
STAGE2_SIZE := 65536

APP_SRCS := $(shell $(PYTHON) tools/gen_image.py --apps-dir apps --ignore apps/.xiaoignore --list-sources)
FILE_SRCS := $(shell $(PYTHON) tools/gen_image.py --files-dir files --files-ignore files/.xiaoignore --list-files)
BIOS_FILE_SRCS := $(shell $(PYTHON) tools/gen_image.py --files-dir files --files-ignore files/.xiaoignore-bios --list-files)
BIOS_APP_OBJS := $(patsubst apps/%.c,$(BUILD)/bios/apps/%.o,$(APP_SRCS))
UEFI_APP_OBJS := $(patsubst apps/%.c,$(BUILD)/uefi/apps/%.obj,$(APP_SRCS))
ARM64_APP_OBJS := $(patsubst apps/%.c,$(BUILD)/arm64/apps/%.obj,$(APP_SRCS))
WRAPPED_APP_SRCS := $(patsubst apps/%.c,$(BUILD)/generated/apps/%.c,$(APP_SRCS))

BIOS_CFLAGS := -m32 -DXIAO_TTF_TERMINAL_DISABLED -Iinclude -Iapps -ffreestanding -fno-stack-protector -fno-pic -fno-pie -mno-sse -mno-mmx -Os -Wall -Wextra
UEFI_CFLAGS := -target x86_64-pc-win32 -DXIAO_UEFI_X86_SERIAL -Iinclude -Iapps -Ihal/uefi -ffreestanding -fshort-wchar -fno-stack-protector -mno-red-zone -Os -Wall -Wextra
ARM64_UEFI_CFLAGS := -target aarch64-unknown-windows -Iinclude -Iapps -Ihal/uefi -ffreestanding -fshort-wchar -fno-stack-protector -Os -Wall -Wextra

.SECONDARY: $(WRAPPED_APP_SRCS)

.PHONY: all pc bios uefi arm64 arduino esp32c3 esp32c3-upload esp32c3-monitor sync-sketches check clean FORCE

all: pc

pc: bios uefi

check: pc arm64

bios: $(BUILD)/bios/xiao-bios.img

uefi: $(BUILD)/uefi/BOOTX64.EFI $(BUILD)/uefi/esp/EFI/BOOT/BOOTX64.EFI

arm64: $(BUILD)/arm64/BOOTAA64.EFI $(BUILD)/arm64/esp/EFI/BOOT/BOOTAA64.EFI

$(BUILD)/bios $(BUILD)/uefi $(BUILD)/uefi/esp/EFI/BOOT $(BUILD)/arm64 $(BUILD)/arm64/esp/EFI/BOOT $(BUILD)/generated:
	mkdir -p $@

$(BUILD)/generated/image.c: boot/common/boot.txt tools/gen_image.py apps/.xiaoignore files/.xiaoignore $(APP_SRCS) $(FILE_SRCS) FORCE | $(BUILD)/generated
	$(PYTHON) tools/gen_image.py --boot boot/common/boot.txt --apps-dir apps --ignore apps/.xiaoignore --files-dir files --files-ignore files/.xiaoignore --out $@

$(BUILD)/generated/image_bios.c: boot/common/boot.txt tools/gen_image.py apps/.xiaoignore files/.xiaoignore-bios $(APP_SRCS) $(BIOS_FILE_SRCS) FORCE | $(BUILD)/generated
	$(PYTHON) tools/gen_image.py --boot boot/common/boot.txt --apps-dir apps --ignore apps/.xiaoignore --files-dir files --files-ignore files/.xiaoignore-bios --out $@

$(BUILD)/generated/apps/%.c: apps/%.c tools/wrap_app.py tools/gen_image.py | $(BUILD)/generated
	mkdir -p $(@D)
	$(PYTHON) tools/wrap_app.py --src $< --app-name "$*" --out $@

$(BUILD)/bios/boot.bin: hal/bios/boot.asm | $(BUILD)/bios
	$(NASM) -f bin -DSTAGE2_SECTORS=$(STAGE2_SECTORS) $< -o $@

$(BUILD)/bios/start32.o: hal/bios/start32.asm | $(BUILD)/bios
	$(NASM) -f elf32 $< -o $@

$(BUILD)/bios/xiao_core.o: kernel/core/xiao_core.c include/xiao.h | $(BUILD)/bios
	$(BIOS_CC) $(BIOS_CFLAGS) -c $< -o $@

$(BUILD)/bios/apps/%.o: $(BUILD)/generated/apps/%.c include/xiao.h | $(BUILD)/bios
	mkdir -p $(@D)
	$(BIOS_CC) $(BIOS_CFLAGS) -c $< -o $@

$(BUILD)/bios/image.o: $(BUILD)/generated/image_bios.c include/xiao.h | $(BUILD)/bios
	$(BIOS_CC) $(BIOS_CFLAGS) -c $< -o $@

$(BUILD)/bios/bios.o: hal/bios/bios.c include/xiao.h | $(BUILD)/bios
	$(BIOS_CC) $(BIOS_CFLAGS) -c $< -o $@

$(BUILD)/bios/stage2.elf: $(BUILD)/bios/start32.o $(BUILD)/bios/xiao_core.o $(BIOS_APP_OBJS) $(BUILD)/bios/image.o $(BUILD)/bios/bios.o hal/bios/linker.ld
	$(BIOS_CC) -m32 -nostdlib -Wl,-m,elf_i386 -Wl,--build-id=none -T hal/bios/linker.ld $(filter %.o,$^) -o $@

$(BUILD)/bios/stage2.raw: $(BUILD)/bios/stage2.elf
	$(BIOS_OBJCOPY) -O binary $< $@

$(BUILD)/bios/stage2.bin: $(BUILD)/bios/stage2.raw
	test $$(wc -c < $<) -le $(STAGE2_SIZE)
	dd if=/dev/zero of=$@ bs=$(STAGE2_SIZE) count=1 2>/dev/null
	dd if=$< of=$@ conv=notrunc 2>/dev/null

$(BUILD)/bios/xiao-bios.img: $(BUILD)/bios/boot.bin $(BUILD)/bios/stage2.bin
	cat $^ > $@

$(BUILD)/uefi/xiao_core.obj: kernel/core/xiao_core.c include/xiao.h | $(BUILD)/uefi
	$(CLANG) $(UEFI_CFLAGS) -c $< -o $@

$(BUILD)/uefi/apps/%.obj: $(BUILD)/generated/apps/%.c include/xiao.h | $(BUILD)/uefi
	mkdir -p $(@D)
	$(CLANG) $(UEFI_CFLAGS) -c $< -o $@

$(BUILD)/uefi/image.obj: $(BUILD)/generated/image.c include/xiao.h | $(BUILD)/uefi
	$(CLANG) $(UEFI_CFLAGS) -c $< -o $@

$(BUILD)/uefi/uefi.obj: hal/uefi/uefi.c hal/uefi/efi.h include/xiao.h | $(BUILD)/uefi
	$(CLANG) $(UEFI_CFLAGS) -c $< -o $@

$(BUILD)/uefi/portio.obj: hal/uefi/portio.asm | $(BUILD)/uefi
	$(NASM) -f win64 $< -o $@

$(BUILD)/uefi/BOOTX64.EFI: $(BUILD)/uefi/xiao_core.obj $(UEFI_APP_OBJS) $(BUILD)/uefi/image.obj $(BUILD)/uefi/uefi.obj $(BUILD)/uefi/portio.obj
	$(LLD_LINK) /subsystem:efi_application /entry:efi_main /nodefaultlib /out:$@ $^

$(BUILD)/uefi/esp/EFI/BOOT/BOOTX64.EFI: $(BUILD)/uefi/BOOTX64.EFI | $(BUILD)/uefi/esp/EFI/BOOT
	cp $< $@

$(BUILD)/arm64/xiao_core.obj: kernel/core/xiao_core.c include/xiao.h | $(BUILD)/arm64
	$(CLANG) $(ARM64_UEFI_CFLAGS) -c $< -o $@

$(BUILD)/arm64/apps/%.obj: $(BUILD)/generated/apps/%.c include/xiao.h | $(BUILD)/arm64
	mkdir -p $(@D)
	$(CLANG) $(ARM64_UEFI_CFLAGS) -c $< -o $@

$(BUILD)/arm64/image.obj: $(BUILD)/generated/image.c include/xiao.h | $(BUILD)/arm64
	$(CLANG) $(ARM64_UEFI_CFLAGS) -c $< -o $@

$(BUILD)/arm64/uefi.obj: hal/uefi/uefi.c hal/uefi/efi.h include/xiao.h | $(BUILD)/arm64
	$(CLANG) $(ARM64_UEFI_CFLAGS) -c $< -o $@

$(BUILD)/arm64/BOOTAA64.EFI: $(BUILD)/arm64/xiao_core.obj $(ARM64_APP_OBJS) $(BUILD)/arm64/image.obj $(BUILD)/arm64/uefi.obj
	$(LLD_LINK) /machine:arm64 /subsystem:efi_application /entry:efi_main /nodefaultlib /out:$@ $^

$(BUILD)/arm64/esp/EFI/BOOT/BOOTAA64.EFI: $(BUILD)/arm64/BOOTAA64.EFI | $(BUILD)/arm64/esp/EFI/BOOT
	cp $< $@

sync-sketches:
	$(PYTHON) tools/sync_sketch.py hal/arduino/xiaoOS
	$(PYTHON) tools/sync_sketch.py hal/esp32c3/xiaoOS

arduino: sync-sketches
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
