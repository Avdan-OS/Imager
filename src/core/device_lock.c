#if !defined(_WIN32)
#define _DEFAULT_SOURCE
#endif

#include "imager/device_lock.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <winioctl.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif

#if defined(_WIN32)

#define MAX_LOCKED_VOLUMES 26

struct device_lock {
    HANDLE volumes[MAX_LOCKED_VOLUMES];
    int count;
};

static int parse_disk_number(const char *dev_path) {
    size_t n = strlen(dev_path);
    size_t d = n;
    while (d > 0 && isdigit((unsigned char)dev_path[d - 1])) d--;
    if (d == n) return -1;
    return atoi(dev_path + d);
}

static int volume_on_disk(HANDLE h, int disk) {
    unsigned char buf[sizeof(VOLUME_DISK_EXTENTS) + 8 * sizeof(DISK_EXTENT)];
    VOLUME_DISK_EXTENTS *ext = (VOLUME_DISK_EXTENTS *)buf;
    DWORD ret = 0;
    DWORD i;

    if (!DeviceIoControl(h, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                         NULL, 0, buf, sizeof(buf), &ret, NULL)) {
        return 0;
    }
    for (i = 0; i < ext->NumberOfDiskExtents; i++) {
        if ((int)ext->Extents[i].DiskNumber == disk) return 1;
    }
    return 0;
}

device_lock_t *device_lock_acquire(const char *dev_path, int exclusive) {
    int disk = parse_disk_number(dev_path);
    device_lock_t *lock;
    char letter;

    if (disk < 0 || strstr(dev_path, "PhysicalDrive") == NULL) {
        fprintf(stderr, "Error: expected a device like \\\\.\\PhysicalDriveN\n");
        return NULL;
    }

    lock = (device_lock_t *)calloc(1, sizeof(*lock));
    if (lock == NULL) {
        perror("calloc");
        return NULL;
    }

    for (letter = 'A'; letter <= 'Z'; letter++) {
        char vol_path[16];
        HANDLE h;
        DWORD ret = 0;
        int attempt;
        int locked = 0;

        snprintf(vol_path, sizeof(vol_path), "\\\\.\\%c:", letter);
        h = CreateFileA(vol_path, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, 0, NULL);
        if (h == INVALID_HANDLE_VALUE) continue;
        if (!volume_on_disk(h, disk)) {
            CloseHandle(h);
            continue;
        }

        printf("Locking and dismounting volume %c: ...\n", letter);
        for (attempt = 0; attempt < 10; attempt++) {
            if (DeviceIoControl(h, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &ret, NULL)) {
                locked = 1;
                break;
            }
            Sleep(500);
        }
        if (!locked) {
            fprintf(stderr, "Error: could not lock volume %c: (in use by another program).\n", letter);
            CloseHandle(h);
            device_lock_release(lock);
            return NULL;
        }
        DeviceIoControl(h, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &ret, NULL);

        if (exclusive && lock->count < MAX_LOCKED_VOLUMES) {
            lock->volumes[lock->count++] = h;
        } else {
            CloseHandle(h);
        }
    }
    return lock;
}

void device_lock_release(device_lock_t *lock) {
    int i;
    DWORD ret = 0;
    if (lock == NULL) return;
    for (i = 0; i < lock->count; i++) {
        DeviceIoControl(lock->volumes[i], FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &ret, NULL);
        CloseHandle(lock->volumes[i]);
    }
    free(lock);
}

#elif defined(__APPLE__)

struct device_lock {
    int unused;
};

device_lock_t *device_lock_acquire(const char *dev_path, int exclusive) {
    char cmd[1024];
    (void)exclusive;

    printf("Unmounting %s (diskutil)...\n", dev_path);
    snprintf(cmd, sizeof(cmd), "diskutil unmountDisk '%s'", dev_path);
    if (system(cmd) != 0) {
        fprintf(stderr, "Error: failed to unmount %s.\n", dev_path);
        return NULL;
    }
    return (device_lock_t *)calloc(1, sizeof(device_lock_t));
}

void device_lock_release(device_lock_t *lock) {
    free(lock);
}

#elif defined(__linux__)

#include <sys/mount.h>

struct device_lock {
    int excl_fd;
};

static int unmount_partitions(const char *dev_path) {
    FILE *f;
    char line[1024];
    char src[512];
    char mnt[512];
    size_t devlen = strlen(dev_path);
    int failures = 0;

    f = fopen("/proc/self/mounts", "r");
    if (f == NULL) {
        return 0;
    }
    while (fgets(line, sizeof(line), f) != NULL) {
        if (sscanf(line, "%511s %511s", src, mnt) != 2) continue;
        if (strncmp(src, dev_path, devlen) != 0) continue;
        printf("Unmounting %s from %s...\n", src, mnt);
        if (umount2(mnt, 0) != 0) {
            fprintf(stderr, "Failed to unmount %s: %s\n", mnt, strerror(errno));
            failures++;
        }
    }
    fclose(f);
    return failures == 0 ? 0 : -1;
}

device_lock_t *device_lock_acquire(const char *dev_path, int exclusive) {
    device_lock_t *lock = (device_lock_t *)calloc(1, sizeof(*lock));
    if (lock == NULL) {
        perror("calloc");
        return NULL;
    }
    lock->excl_fd = -1;

    if (unmount_partitions(dev_path) != 0) {
        free(lock);
        return NULL;
    }

    if (exclusive) {
        lock->excl_fd = open(dev_path, O_RDONLY | O_EXCL);
        if (lock->excl_fd < 0) {
            perror("exclusive open");
            fprintf(stderr, "Device %s is still in use.\n", dev_path);
            free(lock);
            return NULL;
        }
    }
    return lock;
}

void device_lock_release(device_lock_t *lock) {
    if (lock == NULL) return;
    if (lock->excl_fd >= 0) close(lock->excl_fd);
    free(lock);
}

#else

struct device_lock {
    int unused;
};

device_lock_t *device_lock_acquire(const char *dev_path, int exclusive) {
    (void)dev_path;
    (void)exclusive;
    return (device_lock_t *)calloc(1, sizeof(device_lock_t));
}

void device_lock_release(device_lock_t *lock) {
    free(lock);
}

#endif
