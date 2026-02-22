#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#define close _close
#else
#include <unistd.h>
#endif

#include <sys/types.h>
#include "imager/utils.h"
#include "imager/iso_operations.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
            usage(argv[0]);
            return 0;
        }
        basic_usage(argv[0]);
#ifdef _WIN32
        printf("\nNote for Windows users: Device path should look like \\\\.\\PhysicalDriveX\n");
        printf("You can find the drive number in Disk Management or using 'wmic diskdrive list brief'\n");
#endif
        return 1;
    }
    
    const char *iso_path = argv[1];
    const char *dev_path = argv[2];

    if (!confirm_destructive_action(dev_path)) {
        return 1;
    }

    printf("ISO: %s\n", iso_path);

    off_t iso_size;
    int temp_fd = open_iso_file(iso_path, &iso_size);
    if (temp_fd < 0) {
        fprintf(stderr, "Error: Failed to open ISO file '%s'\n", iso_path);
        return 1;
    }
    close(temp_fd);

    if (write_iso_to_device(iso_path, dev_path, print_progress_cli) != 0) {
        fprintf(stderr, "Error: Failed to write ISO to device '%s'\n", dev_path);
        return 1;
    }

    if (verify_device_against_iso(iso_path, dev_path, iso_size, print_progress_cli) != 0) {
        fprintf(stderr, "Error: Verification failed for device '%s'\n", dev_path);
        return 1;
    }

    printf("\n=== Imaging Complete! ===\n");
    printf("ISO '%s' has been successfully written to device '%s'\n", iso_path, dev_path);
    printf("The device is now ready for use.\n\n");

    return 0;
}