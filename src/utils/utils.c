#include "imager/utils.h"
#include <stdio.h>
#include <string.h>

void usage(const char *prog) {
    printf("Usage: %s <iso_file> <usb_device>\n", prog);
    printf("Example: %s linux.iso /dev/sdX\n", prog);
    printf("\nOptions:\n");
    printf("  -h, --help    Show this help message\n");
    printf("\nWARNING: This will completely erase the target device!\n");
    printf("Make sure you have selected the correct device.\n");
}

void basic_usage(const char *prog) {
    printf("Usage: %s <iso_file> <usb_device>\n", prog);
    printf("Use '%s -h' for more information.\n", prog);
}

int confirm_destructive_action(const char *device_path) {
    printf("*** USB ISO Imager ***\n");
    printf("Device: %s\n", device_path);
    printf("\nWARNING: This will erase all data on %s!\n", device_path);
    printf("Type 'YES' to continue: ");
    char confirm[8];
    if (!fgets(confirm, sizeof(confirm), stdin)) {
        printf("Aborted.\n");
        return 0;
    }
    
    confirm[strcspn(confirm, "\n")] = 0;
    
    if (strcmp(confirm, "YES") != 0) {
        printf("Aborted.\n");
        return 0;
    }
    return 1;
} 