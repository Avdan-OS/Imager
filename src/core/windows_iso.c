#if !defined(_WIN32)
#define _DEFAULT_SOURCE
#define _FILE_OFFSET_BITS 64
#endif

#include "imager/windows_iso.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define popen _popen
#define pclose _pclose
#else
#include <unistd.h>
#endif

#define FAT32_MAX_FILE 4294967295ULL
#define TOTAL_STAGES 6

static void stage(progress_callback_t cb, int n, const char *label) {
    printf("\n[%d/%d] %s\n", n, TOTAL_STAGES, label);
    if (cb) cb((off_t)n, (off_t)TOTAL_STAGES, "Extracting");
}

static int run_cmd(const char *fmt, ...) {
    char cmd[4096];
    va_list ap;
    int rc;

    va_start(ap, fmt);
    vsnprintf(cmd, sizeof(cmd), fmt, ap);
    va_end(ap);

    rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "Command failed (exit %d): %s\n", rc, cmd);
    }
    return rc;
}

#if defined(_WIN32)

static int run_capture(char *out, size_t out_len, const char *fmt, ...) {
    char cmd[4096];
    char line[1024];
    va_list ap;
    FILE *p;

    va_start(ap, fmt);
    vsnprintf(cmd, sizeof(cmd), fmt, ap);
    va_end(ap);

    out[0] = '\0';
    p = popen(cmd, "r");
    if (p == NULL) return -1;
    while (fgets(line, sizeof(line), p) != NULL) {
        size_t n = strlen(line);
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r' || line[n - 1] == ' ')) {
            line[--n] = '\0';
        }
        if (n > 0) {
            strncpy(out, line, out_len - 1);
            out[out_len - 1] = '\0';
        }
    }
    pclose(p);
    return out[0] != '\0' ? 0 : -1;
}

int write_iso_extracted(const char *iso_path, const char *dev_path, progress_callback_t cb) {
    size_t n = strlen(dev_path);
    size_t d = n;
    while (d > 0 && isdigit((unsigned char)dev_path[d - 1])) d--;
    if (d == n || strstr(dev_path, "PhysicalDrive") == NULL) {
        fprintf(stderr, "Error: extraction mode expects a device like \\\\.\\PhysicalDriveN\n");
        return -1;
    }
    int disk = atoi(dev_path + d);

    char abs_iso[_MAX_PATH];
    if (_fullpath(abs_iso, iso_path, sizeof(abs_iso)) == NULL) {
        fprintf(stderr, "Error: cannot resolve path '%s'\n", iso_path);
        return -1;
    }

    stage(cb, 1, "Partitioning and formatting target drive (diskpart, FAT32)");
    char script[_MAX_PATH];
    if (GetTempPathA((DWORD)sizeof(script), script) == 0) {
        fprintf(stderr, "GetTempPathA failed\n");
        return -1;
    }
    strncat(script, "imager_diskpart.txt", sizeof(script) - strlen(script) - 1);
    FILE *sf = fopen(script, "w");
    if (sf == NULL) {
        perror("diskpart script");
        return -1;
    }
    fprintf(sf,
            "select disk %d\n"
            "clean\n"
            "convert mbr\n"
            "create partition primary\n"
            "active\n"
            "format fs=fat32 quick label=WINSETUP\n"
            "assign\n"
            "exit\n",
            disk);
    fclose(sf);
    if (run_cmd("diskpart /s \"%s\"", script) != 0) {
        fprintf(stderr, "Note: Windows cannot format FAT32 partitions larger than 32 GB.\n");
        return -1;
    }

    stage(cb, 2, "Mounting ISO");
    char iso_letter[16];
    char usb_letter[16];
    if (run_capture(iso_letter, sizeof(iso_letter),
                    "powershell -NoProfile -Command \"(Mount-DiskImage -ImagePath '%s' -PassThru | Get-Volume).DriveLetter\"",
                    abs_iso) != 0 || !isalpha((unsigned char)iso_letter[0])) {
        fprintf(stderr, "Error: failed to mount ISO '%s'\n", abs_iso);
        return -1;
    }
    if (run_capture(usb_letter, sizeof(usb_letter),
                    "powershell -NoProfile -Command \"(Get-Volume -FileSystemLabel 'WINSETUP' | Select-Object -First 1).DriveLetter\"") != 0
        || !isalpha((unsigned char)usb_letter[0])) {
        fprintf(stderr, "Error: could not locate the freshly formatted WINSETUP volume.\n");
        goto fail;
    }

    {
        char wim[_MAX_PATH];
        struct _stati64 st;
        int split_wim = 0;
        snprintf(wim, sizeof(wim), "%c:\\sources\\install.wim", iso_letter[0]);
        if (_stati64(wim, &st) == 0 && (unsigned long long)st.st_size > FAT32_MAX_FILE) {
            split_wim = 1;
        }

        stage(cb, 3, split_wim ? "Copying files (install.wim will be split)" : "Copying files");
        {
            char cmd[1024];
            int rc;
            snprintf(cmd, sizeof(cmd), "robocopy %c:\\ %c:\\ /E /NFL /NDL /NJH /NJS%s",
                     iso_letter[0], usb_letter[0], split_wim ? " /XF install.wim" : "");
            rc = system(cmd);
            if (rc >= 8) {
                fprintf(stderr, "robocopy failed (exit %d)\n", rc);
                goto fail;
            }
        }

        if (split_wim) {
            stage(cb, 4, "Splitting install.wim into FAT32-sized parts (dism)");
            if (run_cmd("dism /Split-Image /ImageFile:\"%s\" /SWMFile:%c:\\sources\\install.swm /FileSize:3800",
                        wim, usb_letter[0]) != 0) {
                goto fail;
            }
        } else {
            stage(cb, 4, "Copy complete");
        }
    }

    stage(cb, 5, "Dismounting ISO");
    run_cmd("powershell -NoProfile -Command \"Dismount-DiskImage -ImagePath '%s' | Out-Null\"", abs_iso);
    stage(cb, 6, "Done");
    printf("\nExtraction complete. The drive is bootable on UEFI systems.\n");
    return 0;

fail:
    run_cmd("powershell -NoProfile -Command \"Dismount-DiskImage -ImagePath '%s' | Out-Null\"", abs_iso);
    return -1;
}

#elif defined(__APPLE__)

static int find_iso_mount(const char *iso_path, char *mnt, size_t len) {
    char cmd[4096];
    char line[1024];
    FILE *p;

    snprintf(cmd, sizeof(cmd), "hdiutil attach -nobrowse -readonly '%s'", iso_path);
    mnt[0] = '\0';
    p = popen(cmd, "r");
    if (p == NULL) return -1;
    while (fgets(line, sizeof(line), p) != NULL) {
        char *v = strstr(line, "/Volumes/");
        if (v != NULL) {
            size_t n = strlen(v);
            while (n > 0 && (v[n - 1] == '\n' || v[n - 1] == '\r')) v[--n] = '\0';
            strncpy(mnt, v, len - 1);
            mnt[len - 1] = '\0';
        }
    }
    pclose(p);
    return mnt[0] != '\0' ? 0 : -1;
}

int write_iso_extracted(const char *iso_path, const char *dev_path, progress_callback_t cb) {
    char iso_mnt[512];
    char wim[600];
    const char *usb_mnt = "/Volumes/WINSETUP";
    struct stat st;
    int split_wim = 0;
    int ret = -1;

    stage(cb, 1, "Attaching ISO");
    if (find_iso_mount(iso_path, iso_mnt, sizeof(iso_mnt)) != 0) {
        fprintf(stderr, "Error: failed to attach '%s'\n", iso_path);
        return -1;
    }

    stage(cb, 2, "Partitioning and formatting (MBR + FAT32)");
    if (run_cmd("diskutil partitionDisk '%s' MBR \"MS-DOS FAT32\" WINSETUP R", dev_path) != 0) goto cleanup;

    snprintf(wim, sizeof(wim), "%s/sources/install.wim", iso_mnt);
    if (stat(wim, &st) == 0 && (unsigned long long)st.st_size > FAT32_MAX_FILE) {
        if (system("command -v wimlib-imagex >/dev/null 2>&1") != 0) {
            fprintf(stderr,
                    "Error: install.wim exceeds 4 GiB and 'wimlib-imagex' is not installed.\n"
                    "Install it (e.g. 'brew install wimlib') and retry.\n");
            goto cleanup;
        }
        split_wim = 1;
    }

    stage(cb, 3, split_wim ? "Copying files (install.wim will be split)" : "Copying files");
    if (split_wim) {
        if (run_cmd("tar -C '%s' --exclude 'sources/install.wim' -cf - . | tar -C '%s' -xf -", iso_mnt, usb_mnt) != 0) goto cleanup;
        stage(cb, 4, "Splitting install.wim into FAT32-sized parts");
        if (run_cmd("mkdir -p '%s/sources' && wimlib-imagex split '%s' '%s/sources/install.swm' 3800", usb_mnt, wim, usb_mnt) != 0) goto cleanup;
    } else {
        if (run_cmd("tar -C '%s' -cf - . | tar -C '%s' -xf -", iso_mnt, usb_mnt) != 0) goto cleanup;
        stage(cb, 4, "Copy complete");
    }

    stage(cb, 5, "Flushing");
    run_cmd("sync");
    ret = 0;

cleanup:
    stage(cb, 6, "Detaching");
    run_cmd("diskutil unmountDisk '%s' >/dev/null 2>&1 || true", dev_path);
    run_cmd("hdiutil detach '%s' >/dev/null 2>&1 || true", iso_mnt);
    if (ret == 0) {
        printf("\nExtraction complete. The drive is bootable on UEFI systems.\n");
    }
    return ret;
}

#else

static void partition_node(const char *dev, char *out, size_t len) {
    size_t n = strlen(dev);
    if (n > 0 && isdigit((unsigned char)dev[n - 1])) {
        snprintf(out, len, "%sp1", dev);
    } else {
        snprintf(out, len, "%s1", dev);
    }
}

int write_iso_extracted(const char *iso_path, const char *dev_path, progress_callback_t cb) {
    char iso_mnt[] = "/tmp/imager_iso_XXXXXX";
    char usb_mnt[] = "/tmp/imager_usb_XXXXXX";
    char part[512];
    char wim[600];
    struct stat st;
    int split_wim = 0;
    int iso_mounted = 0;
    int usb_mounted = 0;
    int ret = -1;

    if (mkdtemp(iso_mnt) == NULL || mkdtemp(usb_mnt) == NULL) {
        perror("mkdtemp");
        return -1;
    }

    stage(cb, 1, "Partitioning target drive (MBR + FAT32)");
    if (run_cmd("parted -s '%s' mklabel msdos mkpart primary fat32 1MiB 100%% set 1 boot on", dev_path) != 0) goto cleanup;
    run_cmd("partprobe '%s' 2>/dev/null || true", dev_path);
    sleep(1);
    partition_node(dev_path, part, sizeof(part));

    stage(cb, 2, "Formatting FAT32");
    if (run_cmd("mkfs.vfat -F 32 -n WINSETUP '%s'", part) != 0) goto cleanup;

    stage(cb, 3, "Mounting image and target");
    if (run_cmd("mount -o loop,ro '%s' '%s'", iso_path, iso_mnt) != 0) goto cleanup;
    iso_mounted = 1;
    if (run_cmd("mount '%s' '%s'", part, usb_mnt) != 0) goto cleanup;
    usb_mounted = 1;

    snprintf(wim, sizeof(wim), "%s/sources/install.wim", iso_mnt);
    if (stat(wim, &st) == 0 && (unsigned long long)st.st_size > FAT32_MAX_FILE) {
        if (system("command -v wimlib-imagex >/dev/null 2>&1") != 0) {
            fprintf(stderr,
                    "Error: install.wim exceeds 4 GiB and 'wimlib-imagex' is not installed.\n"
                    "Install wimtools (e.g. 'apt install wimtools') and retry.\n");
            goto cleanup;
        }
        split_wim = 1;
    }

    stage(cb, 4, split_wim ? "Copying files (install.wim will be split)" : "Copying files");
    if (split_wim) {
        if (run_cmd("tar -C '%s' --exclude 'sources/install.wim' -cf - . | tar -C '%s' -xf -", iso_mnt, usb_mnt) != 0) goto cleanup;
        stage(cb, 5, "Splitting install.wim into FAT32-sized parts");
        if (run_cmd("mkdir -p '%s/sources' && wimlib-imagex split '%s' '%s/sources/install.swm' 3800", usb_mnt, wim, usb_mnt) != 0) goto cleanup;
    } else {
        if (run_cmd("tar -C '%s' -cf - . | tar -C '%s' -xf -", iso_mnt, usb_mnt) != 0) goto cleanup;
        stage(cb, 5, "Copy complete");
    }

    stage(cb, 6, "Flushing and unmounting");
    run_cmd("sync");
    ret = 0;

cleanup:
    if (usb_mounted) run_cmd("umount '%s'", usb_mnt);
    if (iso_mounted) run_cmd("umount '%s'", iso_mnt);
    rmdir(usb_mnt);
    rmdir(iso_mnt);
    if (ret == 0) {
        printf("\nExtraction complete. The drive is bootable on UEFI systems.\n");
    }
    return ret;
}

#endif
