#!/usr/bin/env sh
# Create a simple disk image
dd if=/dev/zero of=build/xiaoos.img bs=1M count=32
# This is where we would install GRUB to the MBR of this image.
# Without 'grub-install' or 'grub-mkrescue', I cannot make a bootable disk.

echo "Cannot create bootable ISO/Disk without grub-install/xorriso." >&2
exit 1
