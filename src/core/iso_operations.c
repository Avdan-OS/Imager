#include "imager/iso_operations.h"
#include "imager/progress.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#define fsync(fd) _commit(fd)
#define lseek _lseeki64
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#endif

int open_iso_file(const char *iso_path, off_t *iso_size) {
#ifdef _WIN32
    int iso_fd = _open(iso_path, _O_RDONLY | _O_BINARY);
    if (iso_fd < 0) {
        perror("open ISO");
        return -1;
    }
    
    LARGE_INTEGER li;
    HANDLE hFile = (HANDLE)_get_osfhandle(iso_fd);
    if (hFile == INVALID_HANDLE_VALUE || !GetFileSizeEx(hFile, &li)) {
        perror("GetFileSizeEx ISO");
        _close(iso_fd);
        return -1;
    }
    *iso_size = (off_t)li.QuadPart;
    return iso_fd;
#else
    int iso_fd = open(iso_path, O_RDONLY);
    if (iso_fd < 0) {
        perror("open ISO");
        return -1;
    }
    
    struct stat st;
    if (fstat(iso_fd, &st) < 0) {
        perror("fstat ISO");
        close(iso_fd);
        return -1;
    }
    
    *iso_size = st.st_size;
    return iso_fd;
#endif
}

int open_device_file(const char *dev_path, int write_mode) {
#ifdef _WIN32
    DWORD access = GENERIC_READ;
    if (write_mode) access |= GENERIC_WRITE;
    
    HANDLE hDevice = CreateFileA(
        dev_path,
        access,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error opening device %s: %lu\n", dev_path, GetLastError());
        return -1;
    }

    int flags = _O_BINARY;
    if (write_mode) flags |= _O_WRONLY;
    else flags |= _O_RDONLY;

    int dev_fd = _open_osfhandle((intptr_t)hDevice, flags);
    if (dev_fd < 0) {
        CloseHandle(hDevice);
        perror("_open_osfhandle device");
        return -1;
    }
    return dev_fd;
#else
    int flags = write_mode ? O_WRONLY : O_RDONLY;
    int dev_fd = open(dev_path, flags);
    if (dev_fd < 0) {
        perror("open device");
        return -1;
    }
    return dev_fd;
#endif
}

int write_iso_to_device(const char *iso_path, const char *dev_path, progress_callback_t cb) {
    off_t iso_size;
    int iso_fd = open_iso_file(iso_path, &iso_size);
    if (iso_fd < 0) {
        return -1;
    }
    
    int dev_fd = open_device_file(dev_path, 1);
    if (dev_fd < 0) {
#ifdef _WIN32
        _close(iso_fd);
#else
        close(iso_fd);
#endif
        return -1;
    }
    
    printf("\nWriting ISO to device...\n");
#ifdef _WIN32
    _lseeki64(iso_fd, 0, SEEK_SET);
    _lseeki64(dev_fd, 0, SEEK_SET);
#else
    lseek(iso_fd, 0, SEEK_SET);
    lseek(dev_fd, 0, SEEK_SET);
#endif
    
    if (copy_with_progress(iso_fd, dev_fd, iso_size, "Writing ", cb) != 0) {
        fprintf(stderr, "\nWrite failed.\n");
#ifdef _WIN32
        _close(iso_fd);
        _close(dev_fd);
#else
        close(iso_fd);
        close(dev_fd);
#endif
        return -1;
    }
    
#ifdef _WIN32
    _commit(dev_fd);
#else
    fsync(dev_fd);
#endif
    printf("Write complete.\n\n");
    
#ifdef _WIN32
    _close(iso_fd);
    _close(dev_fd);
#else
    close(iso_fd);
    close(dev_fd);
#endif
    return 0;
}

int verify_device_against_iso(const char *iso_path, const char *dev_path, off_t iso_size, progress_callback_t cb) {
#ifdef _WIN32
    int iso_fd = _open(iso_path, _O_RDONLY | _O_BINARY);
#else
    int iso_fd = open(iso_path, O_RDONLY);
#endif
    if (iso_fd < 0) {
        perror("reopen ISO for verify");
        return -1;
    }
    
    int dev_fd = open_device_file(dev_path, 0);
    if (dev_fd < 0) {
#ifdef _WIN32
        _close(iso_fd);
#else
        close(iso_fd);
#endif
        return -1;
    }
    
    printf("Verifying written data...\n");
    if (verify_with_progress(iso_fd, dev_fd, iso_size, cb) != 0) {
        fprintf(stderr, "\nVerification failed.\n");
#ifdef _WIN32
        _close(iso_fd);
        _close(dev_fd);
#else
        close(iso_fd);
        close(dev_fd);
#endif
        return -1;
    }
    
    printf("Verification successful!\n");
#ifdef _WIN32
    _close(iso_fd);
    _close(dev_fd);
#else
    close(iso_fd);
    close(dev_fd);
#endif
    return 0;
}
 