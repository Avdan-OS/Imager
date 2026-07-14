#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include "imager/utils.h"
#include "imager/iso_operations.h"
#include "imager/image_format.h"
#include "imager/image_reader.h"
#include "imager/image_write.h"
#include "imager/windows_iso.h"

static void extra_usage(void) {
    printf("\nOptions:\n");
    printf("  --extract   File-extraction mode for Windows-style install ISOs:\n");
    printf("              partitions the drive (MBR), formats FAT32, copies files,\n");
    printf("              and splits install.wim when it exceeds the FAT32 limit.\n");
    printf("\nCompressed images (.gz, .xz, .bz2, .zst) are decompressed on the fly\n");
    printf("when the matching codec is compiled in.\n");
}

int main(int argc, char *argv[]) {
    int extract_mode = 0;
    const char *iso_path = NULL;
    const char *dev_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            extra_usage();
            return 0;
        } else if (strcmp(argv[i], "--extract") == 0) {
            extract_mode = 1;
        } else if (iso_path == NULL) {
            iso_path = argv[i];
        } else if (dev_path == NULL) {
            dev_path = argv[i];
        } else {
            basic_usage(argv[0]);
            return 1;
        }
    }

    if (iso_path == NULL || dev_path == NULL) {
        basic_usage(argv[0]);
        extra_usage();
#ifdef _WIN32
        printf("\nNote for Windows users: Device path should look like \\\\.\\PhysicalDriveX\n");
        printf("You can find the drive number in Disk Management or using 'wmic diskdrive list brief'\n");
#endif
        return 1;
    }

    printf("Image: %s\n", iso_path);

    image_info_t img_info;
    if (detect_image_format(iso_path, &img_info) != 0) {
        fprintf(stderr, "Error: Failed to analyze image file '%s'\n", iso_path);
        return 1;
    }

    printf("Detected format: %s\n", image_format_name(img_info.format));
    if (img_info.volume_label[0] != '\0') {
        printf("Volume label:    %s\n", img_info.volume_label);
    }
    if (img_info.has_el_torito) {
        printf("Bootable:        yes (El Torito)\n");
    }

    int writable = image_format_is_raw_writable(&img_info);

    if (extract_mode) {
        if (writable < 0) {
            fprintf(stderr, "\nError: extraction mode cannot operate on compressed images.\n"
                            "Decompress the file first, then retry with --extract.\n");
            return 1;
        }
        printf("Mode:            file extraction (Windows-style install media)\n");
    } else if (writable == 0) {
        printf("\nWarning: %s\n", img_info.description);
        if (img_info.format == IMG_FORMAT_UDF || img_info.format == IMG_FORMAT_ISO9660) {
            printf("Hint: for Windows install ISOs, rerun with --extract to copy files\n"
                   "onto a bootable FAT32 partition instead of raw writing.\n");
        }
    } else if (writable < 0) {
        if (!image_reader_format_supported(img_info.format)) {
            fprintf(stderr, "\nError: this build has no %s support compiled in.\n",
                    image_format_name(img_info.format));
            fprintf(stderr, "Rebuild with the codec library installed, or decompress the file manually.\n");
            return 1;
        }
        printf("\nCompressed image detected: streaming decompression will be used.\n");
    }

    if (!confirm_destructive_action(dev_path)) {
        return 1;
    }

    if (extract_mode) {
        if (write_iso_extracted(iso_path, dev_path, print_progress_cli) != 0) {
            fprintf(stderr, "Error: extraction to '%s' failed\n", dev_path);
            return 1;
        }
        printf("\n=== Imaging Complete! ===\n");
        printf("Files from '%s' were extracted to '%s'.\n", iso_path, dev_path);
        printf("(File-level copy: bit-for-bit verification is not applicable.)\n\n");
        return 0;
    }

    off_t bytes_written = 0;
    if (write_image_to_device(iso_path, img_info.format, dev_path, print_progress_cli, &bytes_written) != 0) {
        fprintf(stderr, "Error: Failed to write image to device '%s'\n", dev_path);
        return 1;
    }

    if (verify_device_against_image(iso_path, img_info.format, dev_path, bytes_written, print_progress_cli) != 0) {
        fprintf(stderr, "Error: Verification failed for device '%s'\n", dev_path);
        return 1;
    }

    printf("\n=== Imaging Complete! ===\n");
    printf("Image '%s' has been successfully written to device '%s'\n", iso_path, dev_path);
    printf("The device is now ready for use.\n\n");

    return 0;
}
