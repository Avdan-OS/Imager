#include "imager/iso_operations.h"
#include "imager/progress.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

int open_iso_file(const char *iso_path, off_t *iso_size) {
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
}

int open_device_file(const char *dev_path, int write_mode) {
    int flags = write_mode ? O_WRONLY : O_RDONLY;
    int dev_fd = open(dev_path, flags);
    if (dev_fd < 0) {
        perror("open device");
        return -1;
    }
    return dev_fd;
}

int write_iso_to_device(const char *iso_path, const char *dev_path) {
    off_t iso_size;
    int iso_fd = open_iso_file(iso_path, &iso_size);
    if (iso_fd < 0) {
        return -1;
    }
    
    int dev_fd = open_device_file(dev_path, 1);
    if (dev_fd < 0) {
        close(iso_fd);
        return -1;
    }
    
    printf("\nWriting ISO to device...\n");
    lseek(iso_fd, 0, SEEK_SET);
    lseek(dev_fd, 0, SEEK_SET);
    
    if (copy_with_progress(iso_fd, dev_fd, iso_size, "Writing ") != 0) {
        fprintf(stderr, "\nWrite failed.\n");
        close(iso_fd);
        close(dev_fd);
        return -1;
    }
    
    fsync(dev_fd);
    printf("Write complete.\n\n");
    
    close(iso_fd);
    close(dev_fd);
    return 0;
}

int verify_device_against_iso(const char *iso_path, const char *dev_path, off_t iso_size) {
    int iso_fd = open(iso_path, O_RDONLY);
    if (iso_fd < 0) {
        perror("reopen ISO for verify");
        return -1;
    }
    
    int dev_fd = open_device_file(dev_path, 0);
    if (dev_fd < 0) {
        close(iso_fd);
        return -1;
    }
    
    printf("Verifying written data...\n");
    if (verify_with_progress(iso_fd, dev_fd, iso_size) != 0) {
        fprintf(stderr, "\nVerification failed.\n");
        close(iso_fd);
        close(dev_fd);
        return -1;
    }
    
    printf("Verification successful!\n");
    close(iso_fd);
    close(dev_fd);
    return 0;
} 