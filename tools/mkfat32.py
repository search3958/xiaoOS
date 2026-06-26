#!/usr/bin/env python3
"""Create a FAT32 boot image for Raspberry Pi 4 with MBR partition table."""
import struct
import os
import sys

SECTOR_SIZE = 512
FAT32_EOC = 0x0FFFFFF8


def align_up(value, alignment):
    return (value + alignment - 1) // alignment * alignment


def make_fat32_image(output_path, files, image_size_mb=64):
    image_size = image_size_mb * 1024 * 1024
    image = bytearray(image_size)

    PART_START = 2048
    PART_SIZE = (image_size // SECTOR_SIZE) - PART_START

    image[510:512] = b'\x55\xAA'
    image[446] = 0x00
    image[447] = 0x00
    image[448] = 0x21
    image[449] = 0x00
    image[450] = 0x00
    image[451] = 0x0C
    image[452] = 0xFE
    image[453] = 0xFF
    image[454] = 0xFF
    struct.pack_into('<I', image, 455, PART_START)
    struct.pack_into('<I', image, 459, PART_SIZE)

    fat_base = PART_START * SECTOR_SIZE
    bps = SECTOR_SIZE
    spc = 8
    reserved = 32
    num_fats = 2
    root_cluster = 2

    total_sectors_fat32 = PART_SIZE
    data_area = total_sectors_fat32 - reserved - num_fats * 128
    clusters = data_area // spc
    fat_size_sectors = max(128, align_up((clusters + 2) * 4, bps) // bps)

    image[fat_base + 0:fat_base + 3] = b'\xEB\x58\x90'
    image[fat_base + 3:fat_base + 11] = b'MSWIN4.1'
    struct.pack_into('<H', image, fat_base + 11, bps)
    image[fat_base + 13] = spc
    struct.pack_into('<H', image, fat_base + 14, reserved)
    image[fat_base + 16] = num_fats
    struct.pack_into('<H', image, fat_base + 17, 0)
    struct.pack_into('<H', image, fat_base + 19, 0)
    image[fat_base + 21] = 0xF8
    struct.pack_into('<H', image, fat_base + 22, 0)
    struct.pack_into('<H', image, fat_base + 24, 63)
    struct.pack_into('<H', image, fat_base + 26, 255)
    struct.pack_into('<I', image, fat_base + 28, 0)
    struct.pack_into('<I', image, fat_base + 32, total_sectors_fat32)
    struct.pack_into('<I', image, fat_base + 36, fat_size_sectors)
    struct.pack_into('<H', image, fat_base + 40, 0)
    struct.pack_into('<H', image, fat_base + 42, 0)
    struct.pack_into('<I', image, fat_base + 44, root_cluster)
    struct.pack_into('<H', image, fat_base + 48, 1)
    struct.pack_into('<H', image, fat_base + 50, 6)
    image[fat_base + 64] = 0x80
    image[fat_base + 66] = 0x29
    image[fat_base + 67:fat_base + 71] = b'\x12\x34\x56\x78'
    image[fat_base + 71:fat_base + 82] = b'XIAO_OS    '
    image[fat_base + 82:fat_base + 90] = b'FAT32   '
    image[fat_base + 510:fat_base + 512] = b'\x55\xAA'

    fat_start_byte = fat_base + reserved * bps
    data_start = fat_base + (reserved + num_fats * fat_size_sectors) * bps

    struct.pack_into('<I', image, fat_start_byte + root_cluster * 4, FAT32_EOC)

    _next_free = 3
    allocated = set()

    def cluster_offset(cl):
        return data_start + (cl - 2) * spc * bps

    def alloc_clusters(count):
        nonlocal _next_free
        clusters = []
        for _ in range(count):
            while _next_free in allocated:
                _next_free += 1
            clusters.append(_next_free)
            allocated.add(_next_free)
            _next_free += 1
        for i in range(len(clusters) - 1):
            struct.pack_into('<I', image, fat_start_byte + clusters[i] * 4, clusters[i + 1])
        if clusters:
            struct.pack_into('<I', image, fat_start_byte + clusters[-1] * 4, FAT32_EOC)
        return clusters

    def make_dir_entry(name_8, ext_3, attr, first_cluster, size):
        e = bytearray(32)
        e[0:8] = name_8.upper().ljust(8)[:8].encode('ascii')
        e[8:11] = ext_3.upper().ljust(3)[:3].encode('ascii')
        e[11] = attr
        e[20] = (first_cluster >> 16) & 0xFF
        e[26] = first_cluster & 0xFF
        e[27] = (first_cluster >> 8) & 0xFF
        struct.pack_into('<I', e, 28, size)
        return e

    # ── Phase 1: Allocate all clusters for directories and files ──────────
    dir_clusters = {}  # path -> first_cluster
    file_clusters = {}  # dest_name -> (first_cluster, file_size, src_path)
    dir_parents = {}  # path -> parent_path

    dir_clusters['/'] = root_cluster

    for src_path, dest_name in files:
        parts = dest_name.replace('\\', '/').split('/')
        dirs = parts[:-1]
        filename = parts[-1]

        # Ensure all parent directories exist
        current_path = '/'
        for d in dirs:
            parent_path = current_path
            child_path = current_path + d + '/'
            if child_path not in dir_clusters:
                cls = alloc_clusters(1)
                dir_clusters[child_path] = cls[0]
                dir_parents[child_path] = parent_path
                # Write . and .. entries
                dir_off = cluster_offset(cls[0])
                image[dir_off:dir_off + 32] = make_dir_entry('.', '', 0x10, cls[0], 0)
                image[dir_off + 32:dir_off + 64] = make_dir_entry('..', '', 0x10, dir_clusters[parent_path], 0)
            current_path = child_path

        # Allocate clusters for file data
        file_size = os.path.getsize(src_path)
        num_clusters = max(1, (file_size + spc * bps - 1) // (spc * bps))
        cls = alloc_clusters(num_clusters)
        file_clusters[dest_name] = (cls[0], file_size, src_path)

    # ── Phase 2: Write directory entries for all directories ──────────────
    for child_path, parent_path in dir_parents.items():
        child_name = child_path.rstrip('/').split('/')[-1]
        parent_cluster = dir_clusters[parent_path]
        child_cluster = dir_clusters[child_path]

        parent_off = cluster_offset(parent_cluster)
        for i in range(spc * bps // 32):
            e = image[parent_off + i * 32: parent_off + (i + 1) * 32]
            if e[0] == 0 or e[0] == 0xE5:
                image[parent_off + i * 32: parent_off + (i + 1) * 32] = make_dir_entry(child_name, '', 0x10, child_cluster, 0)
                break

    # ── Phase 3: Write file data and directory entries ────────────────────
    for dest_name, (first_cluster, file_size, src_path) in file_clusters.items():
        file_data = open(src_path, 'rb').read()

        # Write data to clusters
        clusters_list = []
        c = first_cluster
        while c < FAT32_EOC:
            clusters_list.append(c)
            c = struct.unpack_from('<I', image, fat_start_byte + c * 4)[0] & 0x0FFFFFFF

        for ci, cl in enumerate(clusters_list):
            off = cluster_offset(cl)
            start = ci * spc * bps
            end = min(start + spc * bps, file_size)
            image[off:off + end - start] = file_data[start:end]

        # Add directory entry
        parts = dest_name.replace('\\', '/').split('/')
        dirs = parts[:-1]
        filename = parts[-1]
        if '.' in filename:
            name, ext = filename.rsplit('.', 1)
        else:
            name, ext = filename, ''

        parent_path = '/'
        for d in dirs:
            parent_path += d + '/'

        dir_off = cluster_offset(dir_clusters[parent_path])
        for i in range(spc * bps // 32):
            e = image[dir_off + i * 32: dir_off + (i + 1) * 32]
            if e[0] == 0 or e[0] == 0xE5:
                image[dir_off + i * 32: dir_off + (i + 1) * 32] = make_dir_entry(name, ext, 0x20, first_cluster, file_size)
                break

    with open(output_path, 'wb') as f:
        f.write(image)

    return len(image)


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(f'Usage: {sys.argv[0]} <output.img> <file1:dest1> [file2:dest2] ...', file=sys.stderr)
        sys.exit(1)

    output = sys.argv[1]
    files = []
    for arg in sys.argv[2:]:
        src, dest = arg.split(':', 1)
        files.append((src, dest))

    size = make_fat32_image(output, files)
    print(f'Created {output} ({size} bytes, {size // 1024 // 1024} MiB)')
