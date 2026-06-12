BUILD := build
PYTHON := python3

# Tool auto-detection (Linux/macOS). You can still override via env, e.g. CLANG=clang-18.
detect_tool = $(firstword $(foreach bin,$(1),$(if $(shell command -v $(bin) 2>/dev/null),$(bin))))

BIOS_CC ?= $(call detect_tool,x86_64-elf-gcc i686-elf-gcc gcc)
BIOS_OBJCOPY ?= $(call detect_tool,x86_64-elf-objcopy i686-elf-objcopy objcopy llvm-objcopy)
CLANG ?= $(call detect_tool,clang clang-21 clang-20 clang-19 clang-18 clang-17 clang-16 clang-15)
LLD_LINK_FRONTEND := $(call detect_tool,lld-link lld-link-21 lld-link-20 lld-link-19 lld-link-18 lld-link-17 lld-link-16 lld-link-15)
LD_LLD := $(call detect_tool,ld.lld ld.lld-21 ld.lld-20 ld.lld-19 ld.lld-18 ld.lld-17 ld.lld-16 ld.lld-15)
ifneq ($(strip $(LLD_LINK_FRONTEND)),)
LLD_LINK_BIN := $(LLD_LINK_FRONTEND)
LLD_LINK ?= $(LLD_LINK_FRONTEND)
else
LLD_LINK_BIN := $(LD_LLD)
LLD_LINK ?= $(if $(strip $(LD_LLD)),$(LD_LLD) -flavor link,)
endif
NASM ?= $(call detect_tool,nasm)
GRUB_MKRESCUE ?= $(call detect_tool,i686-elf-grub-mkrescue grub-mkrescue)
STAGE2_SECTORS := 127
STAGE2_SIZE := 65024

BIOS_APP_SRCS := $(shell $(PYTHON) tools/gen_image.py --apps-dir apps --ignore apps/.xiaoignore-bios --list-sources)
UEFI_APP_SRCS := $(shell $(PYTHON) tools/gen_image.py --apps-dir apps --ignore apps/.xiaoignore --list-sources)
FILE_SRCS := $(shell $(PYTHON) tools/gen_image.py --files-dir files --files-ignore files/.xiaoignore --list-files)
BIOS_FILE_SRCS := $(shell $(PYTHON) tools/gen_image.py --files-dir files --files-ignore files/.xiaoignore-bios --list-files)

BIOS_APP_OBJS := $(patsubst apps/%.c,$(BUILD)/bios/apps/%.o,$(BIOS_APP_SRCS))
UEFI_APP_OBJS := $(patsubst apps/%.c,$(BUILD)/uefi/apps/%.obj,$(UEFI_APP_SRCS))
LIMINE_APP_OBJS := $(patsubst apps/%.c,$(BUILD)/limine/apps/%.o,$(UEFI_APP_SRCS))
GRUB_APP_OBJS := $(patsubst apps/%.c,$(BUILD)/grub/apps/%.o,$(UEFI_APP_SRCS))
ARM64_APP_OBJS := $(patsubst apps/%.c,$(BUILD)/arm64/apps/%.obj,$(UEFI_APP_SRCS))
WRAPPED_APP_SRCS := $(patsubst apps/%.c,$(BUILD)/generated/apps/%.c,$(UEFI_APP_SRCS))

BIOS_CFLAGS := -m32 -Iinclude -Iapps -ffreestanding -fno-stack-protector -fno-pic -fno-pie -mno-sse -mno-mmx -Os -Wall -Wextra -ffunction-sections -fdata-sections -DXIAO_BIOS
UEFI_CFLAGS := -target x86_64-pc-win32 -DXIAO_UEFI_X86_SERIAL -Iinclude -Iapps -Ihal/uefi -ffreestanding -fshort-wchar -fno-stack-protector -mno-red-zone -Os -Wall -Wextra
LIMINE_CFLAGS := -target x86_64-elf -Iinclude -Iapps -Ihal/limine -ffreestanding -fno-stack-protector -mno-red-zone -mcmodel=kernel -Os -Wall -Wextra -DXIAO_LIMINE
GRUB_CFLAGS := -target x86_64-elf -Iinclude -Iapps -Ihal/grub -ffreestanding -fno-stack-protector -mno-red-zone -Os -Wall -Wextra -DXIAO_GRUB
ARM64_UEFI_CFLAGS := -target aarch64-unknown-windows -Iinclude -Iapps -Ihal/uefi -ffreestanding -fshort-wchar -fno-stack-protector -Os -Wall -Wextra

.SECONDARY: $(WRAPPED_APP_SRCS)

.PHONY: all pc bios uefi limine grub arm64 arduino esp32c3 esp32c3-upload esp32c3-monitor sync-sketches check clean FORCE toolchain-check-bios toolchain-check-uefi toolchain-check-limine toolchain-check-grub

all: pc

pc: grub

check: pc arm64

limine: toolchain-check-limine $(BUILD)/xiaoOS.iso

grub: toolchain-check-grub $(BUILD)/xiaoOS-grub.iso

bios: toolchain-check-bios $(BUILD)/bios/xiao-bios.img

uefi: toolchain-check-uefi $(BUILD)/uefi/BOOTX64.EFI $(BUILD)/uefi/esp/EFI/BOOT/BOOTX64.EFI

arm64: toolchain-check-uefi $(BUILD)/arm64/BOOTAA64.EFI $(BUILD)/arm64/esp/EFI/BOOT/BOOTAA64.EFI

toolchain-check-limine:
	@[ -n "$(strip $(CLANG))" ] || { \
		echo "Missing required tool: clang" >&2; \
		exit 127; \
	}
	@[ -d "limine-bin-git" ] || { \
		echo "Limine binaries not found. Please run 'git clone https://github.com/limine-bootloader/limine.git --branch=v8.4.0-binary --depth=1 limine-bin-git'" >&2; \
		exit 1; \
	}

toolchain-check-grub:
	@[ -n "$(strip $(CLANG))" ] || { \
		echo "Missing required tool: clang" >&2; \
		exit 127; \
	}
	@[ -n "$(strip $(GRUB_MKRESCUE))" ] || { \
		echo "Missing required tool: grub-mkrescue (or i686-elf-grub-mkrescue)" >&2; \
		exit 127; \
	}
	@[ -n "$(strip $(NASM))" ] || { \
		echo "Missing required tool: nasm" >&2; \
		exit 127; \
	}

toolchain-check-bios:
	@[ -n "$(strip $(NASM))" ] || { \
		echo "Missing required tool: nasm" >&2; \
		echo "Ubuntu/Debian: sudo apt install nasm" >&2; \
		exit 127; \
	}
	@[ -n "$(strip $(BIOS_CC))" ] || { \
		echo "No C compiler found for BIOS build." >&2; \
		echo "Install x86_64-elf-gcc (recommended) or gcc with 32-bit support." >&2; \
		echo "Ubuntu/Debian: sudo apt install gcc-multilib" >&2; \
		exit 127; \
	}
	@[ -n "$(strip $(BIOS_OBJCOPY))" ] || { \
		echo "Missing required tool: objcopy (or llvm-objcopy)." >&2; \
		echo "Ubuntu/Debian: sudo apt install binutils" >&2; \
		exit 127; \
	}

toolchain-check-uefi:
	@[ -n "$(strip $(CLANG))" ] || { \
		echo "Missing required tool: clang" >&2; \
		echo "Ubuntu/Debian: sudo apt install clang lld nasm" >&2; \
		exit 127; \
	}
	@[ -n "$(strip $(LLD_LINK_BIN))" ] || { \
		echo "Missing required tool: lld-link (or ld.lld)." >&2; \
		echo "Ubuntu/Debian: sudo apt install lld" >&2; \
		exit 127; \
	}
	@[ -n "$(strip $(NASM))" ] || { \
		echo "Missing required tool: nasm" >&2; \
		echo "Ubuntu/Debian: sudo apt install nasm" >&2; \
		exit 127; \
	}

$(BUILD)/bios $(BUILD)/uefi $(BUILD)/uefi/esp/EFI/BOOT $(BUILD)/arm64 $(BUILD)/arm64/esp/EFI/BOOT $(BUILD)/generated:
	mkdir -p $@

$(BUILD)/generated/image.c: boot/common/boot.txt tools/gen_image.py apps/.xiaoignore files/.xiaoignore $(APP_SRCS) $(FILE_SRCS) FORCE | $(BUILD)/generated
	$(PYTHON) tools/gen_image.py --boot boot/common/boot.txt --apps-dir apps --ignore apps/.xiaoignore --files-dir files --files-ignore files/.xiaoignore --out $@

$(BUILD)/generated/image_bios.c: boot/common/boot.txt tools/gen_image.py apps/.xiaoignore-bios files/.xiaoignore-bios $(APP_SRCS) $(BIOS_FILE_SRCS) FORCE | $(BUILD)/generated
	$(PYTHON) tools/gen_image.py --boot boot/common/boot.txt --apps-dir apps --ignore apps/.xiaoignore-bios --files-dir files --files-ignore files/.xiaoignore-bios --out $@

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
	$(BIOS_CC) -m32 -nostdlib -Wl,-m,elf_i386 -Wl,--build-id=none -Wl,--gc-sections -T hal/bios/linker.ld $(filter %.o,$^) -o $@

$(BUILD)/bios/stage2.raw: $(BUILD)/bios/stage2.elf
	$(BIOS_OBJCOPY) --strip-all -O binary $< $@

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

# Limine Rules
$(BUILD)/limine/xiao_core.o: kernel/core/xiao_core.c include/xiao.h | $(BUILD)/limine
	$(CLANG) $(LIMINE_CFLAGS) -c $< -o $@

$(BUILD)/limine/apps/%.o: $(BUILD)/generated/apps/%.c include/xiao.h | $(BUILD)/limine
	mkdir -p $(@D)
	$(CLANG) $(LIMINE_CFLAGS) -c $< -o $@

$(BUILD)/limine/image.o: $(BUILD)/generated/image.c include/xiao.h | $(BUILD)/limine
	$(CLANG) $(LIMINE_CFLAGS) -c $< -o $@

$(BUILD)/limine/limine_hal.o: hal/limine/limine_hal.c hal/limine/limine.h include/xiao.h | $(BUILD)/limine
	$(CLANG) $(LIMINE_CFLAGS) -c $< -o $@

$(BUILD)/limine/xiao-kernel.elf: $(BUILD)/limine/xiao_core.o $(LIMINE_APP_OBJS) $(BUILD)/limine/image.o $(BUILD)/limine/limine_hal.o hal/limine/linker.ld
	$(LD_LLD) -T hal/limine/linker.ld $(filter %.o,$^) -o $@

limine-bin-git/limine-tool: limine-bin-git/limine.c
	$(CC) -O2 $< -o $@

$(BUILD)/xiaoOS.iso: $(BUILD)/limine/xiao-kernel.elf limine.conf limine-bin-git/limine-tool | $(BUILD)/limine
	rm -rf $(BUILD)/iso_root
	mkdir -p $(BUILD)/iso_root
	cp $(BUILD)/limine/xiao-kernel.elf limine.conf \
	   limine-bin-git/limine-bios.sys limine-bin-git/limine-bios-cd.bin limine-bin-git/limine-uefi-cd.bin \
	   $(BUILD)/iso_root/
	mkdir -p $(BUILD)/iso_root/EFI/BOOT
	cp limine-bin-git/BOOTX64.EFI $(BUILD)/iso_root/EFI/BOOT/
	cp limine-bin-git/BOOTAA64.EFI $(BUILD)/iso_root/EFI/BOOT/
	xorriso -as mkisofs -b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(BUILD)/iso_root -o $@
	./limine-bin-git/limine-tool bios-install $@

# GRUB Rules
$(BUILD)/grub/xiao_core.o: kernel/core/xiao_core.c include/xiao.h | $(BUILD)/grub
	$(CLANG) $(GRUB_CFLAGS) -c $< -o $@

$(BUILD)/grub/apps/%.o: $(BUILD)/generated/apps/%.c include/xiao.h | $(BUILD)/grub
	mkdir -p $(@D)
	$(CLANG) $(GRUB_CFLAGS) -c $< -o $@

$(BUILD)/grub/image.o: $(BUILD)/generated/image.c include/xiao.h | $(BUILD)/grub
	$(CLANG) $(GRUB_CFLAGS) -c $< -o $@

$(BUILD)/grub/grub_hal.o: hal/grub/grub_hal.c hal/grub/multiboot2.h include/xiao.h | $(BUILD)/grub
	$(CLANG) $(GRUB_CFLAGS) -c $< -o $@

$(BUILD)/grub/start.o: hal/grub/start.asm | $(BUILD)/grub
	$(NASM) -f elf64 $< -o $@

$(BUILD)/grub/xiao-kernel.elf: $(BUILD)/grub/start.o $(BUILD)/grub/xiao_core.o $(GRUB_APP_OBJS) $(BUILD)/grub/image.o $(BUILD)/grub/grub_hal.o hal/grub/linker.ld
	$(LD_LLD) -T hal/grub/linker.ld $(filter %.o,$^) -o $@

$(BUILD)/xiaoOS-grub.iso: $(BUILD)/grub/xiao-kernel.elf hal/grub/grub.cfg
	rm -rf $(BUILD)/grub_root
	mkdir -p $(BUILD)/grub_root/boot/grub
	cp $(BUILD)/grub/xiao-kernel.elf $(BUILD)/grub_root/boot/
	cp hal/grub/grub.cfg $(BUILD)/grub_root/boot/grub/
	$(GRUB_MKRESCUE) -o $@ $(BUILD)/grub_root

$(BUILD)/limine $(BUILD)/grub:
	mkdir -p $@

sync-sketches:
	$(PYTHON) tools/sync_sketch.py hal/arduino/xiaoOS
	$(PYTHON) tools/sync_sketch.py hal/esp32c3/xiaoOS

arduino: sync-sketches
	@command -v arduino-cli >/dev/null || { echo "arduino-cli is required for this target"; exit 127; }
	arduino-cli compile --fqbn arduino:avr:uno --build-property build.extra_flags="-DXIAO_TTF_TERMINAL_DISABLED" hal/arduino/xiaoOS

esp32c3:
	./esp32c3.sh compile

esp32c3-upload:
	./esp32c3.sh upload

esp32c3-monitor:
	./esp32c3.sh monitor

clean:
	rm -rf $(BUILD)
