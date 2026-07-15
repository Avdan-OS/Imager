#include "imager/image_format.h"

#include <stdio.h>
#include <string.h>

#define SECTOR_SIZE      512
#define ISO_SECTOR_SIZE  2048
#define VD_FIRST_SECTOR  16
#define VD_LAST_SECTOR   48

static int read_at(FILE *f, long offset, unsigned char *buf, size_t len) {
    if (fseek(f, offset, SEEK_SET) != 0) {
        return -1;
    }
    if (fread(buf, 1, len, f) != len) {
        return -1;
    }
    return 0;
}

static void set_desc(image_info_t *info, const char *desc) {
    strncpy(info->description, desc, sizeof(info->description) - 1);
    info->description[sizeof(info->description) - 1] = '\0';
}

const char *image_format_name(image_format_t fmt) {
    switch (fmt) {
        case IMG_FORMAT_ISO_HYBRID:      return "Hybrid ISO (ISO9660 + MBR/GPT)";
        case IMG_FORMAT_ISO9660:         return "Plain ISO9660 (no partition table)";
        case IMG_FORMAT_UDF:             return "UDF";
        case IMG_FORMAT_RAW_MBR:         return "Raw disk image (MBR)";
        case IMG_FORMAT_RAW_GPT:         return "Raw disk image (GPT)";
        case IMG_FORMAT_COMPRESSED_GZ:   return "gzip-compressed image (.gz)";
        case IMG_FORMAT_COMPRESSED_XZ:   return "xz-compressed image (.xz)";
        case IMG_FORMAT_COMPRESSED_BZ2:  return "bzip2-compressed image (.bz2)";
        case IMG_FORMAT_COMPRESSED_ZSTD: return "zstd-compressed image (.zst)";
        case IMG_FORMAT_UNKNOWN:
        default:                         return "Unknown";
    }
}

int image_format_is_raw_writable(const image_info_t *info) {
    switch (info->format) {
        case IMG_FORMAT_ISO_HYBRID:
        case IMG_FORMAT_RAW_MBR:
        case IMG_FORMAT_RAW_GPT:
            return 1;
        case IMG_FORMAT_ISO9660:
        case IMG_FORMAT_UDF:
        case IMG_FORMAT_UNKNOWN:
            return 0;
        default:
            return -1;
    }
}

static image_format_t detect_compression(const unsigned char *b) {
    if (b[0] == 0x1F && b[1] == 0x8B) {
        return IMG_FORMAT_COMPRESSED_GZ;
    }
    if (memcmp(b, "\xFD" "7zXZ\x00", 6) == 0) {
        return IMG_FORMAT_COMPRESSED_XZ;
    }
    if (b[0] == 'B' && b[1] == 'Z' && b[2] == 'h') {
        return IMG_FORMAT_COMPRESSED_BZ2;
    }
    if (b[0] == 0x28 && b[1] == 0xB5 && b[2] == 0x2F && b[3] == 0xFD) {
        return IMG_FORMAT_COMPRESSED_ZSTD;
    }
    return IMG_FORMAT_UNKNOWN;
}

static void extract_volume_label(image_info_t *info, const unsigned char *pvd) {
    int i;
    memcpy(info->volume_label, pvd + 40, 32);
    info->volume_label[32] = '\0';
    for (i = 31; i >= 0; i--) {
        if (info->volume_label[i] == ' ' || info->volume_label[i] == '\0') {
            info->volume_label[i] = '\0';
        } else {
            break;
        }
    }
}

int detect_image_format(const char *path, image_info_t *info) {
    FILE *f;
    unsigned char sector[ISO_SECTOR_SIZE];
    image_format_t compressed;
    int s, i;

    memset(info, 0, sizeof(*info));
    info->format = IMG_FORMAT_UNKNOWN;

    f = fopen(path, "rb");
    if (f == NULL) {
        perror("open image");
        return -1;
    }

    if (read_at(f, 0, sector, SECTOR_SIZE) != 0) {
        fprintf(stderr, "Error: image is smaller than one sector\n");
        fclose(f);
        return -1;
    }

    compressed = detect_compression(sector);
    if (compressed != IMG_FORMAT_UNKNOWN) {
        info->format = compressed;
        set_desc(info, "This is a compressed container, not a raw image. "
                       "Writing it directly would produce an unusable drive.");
        fclose(f);
        return 0;
    }

    if (sector[510] == 0x55 && sector[511] == 0xAA) {
        for (i = 0; i < 4; i++) {
            unsigned char part_type = sector[446 + i * 16 + 4];
            if (part_type != 0x00) {
                info->has_mbr = 1;
                break;
            }
        }
    }

    if (read_at(f, SECTOR_SIZE, sector, SECTOR_SIZE) == 0 &&
        memcmp(sector, "EFI PART", 8) == 0) {
        info->has_gpt = 1;
    }

    for (s = VD_FIRST_SECTOR; s < VD_LAST_SECTOR; s++) {
        if (read_at(f, (long)s * ISO_SECTOR_SIZE, sector, ISO_SECTOR_SIZE) != 0) {
            break;
        }
        if (memcmp(sector + 1, "CD001", 5) == 0) {
            info->has_iso9660 = 1;
            if (sector[0] == 0x01) {
                extract_volume_label(info, sector);
            } else if (sector[0] == 0x00 &&
                       memcmp(sector + 7, "EL TORITO SPECIFICATION", 23) == 0) {
                info->has_el_torito = 1;
            }
        } else if (memcmp(sector + 1, "NSR02", 5) == 0 ||
                   memcmp(sector + 1, "NSR03", 5) == 0) {
            info->has_udf = 1;
        }
    }

    fclose(f);

    if (info->has_iso9660 && (info->has_mbr || info->has_gpt)) {
        info->format = IMG_FORMAT_ISO_HYBRID;
        set_desc(info, "Hybrid ISO with a partition table. Safe to write raw; "
                       "the drive will boot on BIOS and/or UEFI systems.");
    } else if (info->has_iso9660) {
        info->format = IMG_FORMAT_ISO9660;
        set_desc(info, "Plain ISO9660 image without a partition table. A raw "
                       "write may not boot on all systems. Windows install "
                       "ISOs require file extraction instead (not yet supported).");
    } else if (info->has_udf) {
        info->format = IMG_FORMAT_UDF;
        set_desc(info, "UDF image without a partition table (typical of Windows "
                       "install media). A raw write is unlikely to produce a "
                       "bootable drive.");
    } else if (info->has_gpt) {
        info->format = IMG_FORMAT_RAW_GPT;
        set_desc(info, "Raw disk image with a GPT partition table. Safe to write raw.");
    } else if (info->has_mbr) {
        info->format = IMG_FORMAT_RAW_MBR;
        set_desc(info, "Raw disk image with an MBR partition table. Safe to write raw.");
    } else {
        info->format = IMG_FORMAT_UNKNOWN;
        set_desc(info, "No recognizable image signature found. The data will be "
                       "written as-is, but the result may not be bootable.");
    }

    return 0;
}
