#ifndef DRIVE_LIST_H
#define DRIVE_LIST_H

#include <sys/types.h>

typedef struct {
    char path[256];      // System path (e.g., \\.\PhysicalDrive1 or /dev/sdb)
    char name[256];      // Friendly name (e.g., "Samsung Flash Drive")
    long long size;      // Size in bytes
    int is_removable;    // 1 if removable, 0 otherwise
} drive_info_t;

/**
 * Lists available drives on the system.
 * @param drives Array to be populated.
 * @param max_drives Maximum number of drives to list.
 * @return Number of drives found, or -1 on error.
 */
int list_available_drives(drive_info_t *drives, int max_drives);

#endif
