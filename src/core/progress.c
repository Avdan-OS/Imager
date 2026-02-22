#include "imager/progress.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#define read _read
#define write _write
#define lseek _lseeki64
#else
#include <unistd.h>
#endif

#define BLOCK_SIZE 65536 // 64KB

void print_progress_cli(off_t done, off_t total, const char *label) {
    int width = 50;
    float percent = (float)done / total;
    int pos = (int)(percent * width);
    printf("\r%s [", label);
    for (int i = 0; i < width; ++i) {
        if (i < pos) printf("#");
        else if (i == pos) printf(">");
        else printf(" ");
    }
    printf("] %6.2f%%", percent * 100);
    fflush(stdout);
}

int copy_with_progress(int in_fd, int out_fd, off_t total_size, const char *label, progress_callback_t cb) {
    char *buffer = malloc(BLOCK_SIZE);
    if (!buffer) {
        perror("malloc");
        return -1;
    }
    off_t copied = 0;
    int r, w;
    while ((r = read(in_fd, buffer, BLOCK_SIZE)) > 0) {
        w = write(out_fd, buffer, r);
        if (w != r) {
            perror("write");
            free(buffer);
            return -1;
        }
        copied += r;
        if (cb) cb(copied, total_size, label);
    }
    if (cb) cb(total_size, total_size, label);
    free(buffer);
    if (r < 0) {
        perror("read");
        return -1;
    }
    return 0;
}

int verify_with_progress(int iso_fd, int dev_fd, off_t total_size, progress_callback_t cb) {
    char *buf1 = malloc(BLOCK_SIZE);
    char *buf2 = malloc(BLOCK_SIZE);
    if (!buf1 || !buf2) {
        perror("malloc");
        free(buf1); free(buf2);
        return -1;
    }
    off_t compared = 0;
    int r1, r2;
    lseek(iso_fd, 0, SEEK_SET);
    lseek(dev_fd, 0, SEEK_SET);
    while ((r1 = read(iso_fd, buf1, BLOCK_SIZE)) > 0) {
        r2 = read(dev_fd, buf2, r1);
        if (r2 != r1) {
            fprintf(stderr, "Verification failed: device shorter than ISO.\n");
            free(buf1); free(buf2);
            return -1;
        }
        if (memcmp(buf1, buf2, r1) != 0) {
            fprintf(stderr, "Verification failed: data mismatch.\n");
            free(buf1); free(buf2);
            return -1;
        }
        compared += r1;
        if (cb) cb(compared, total_size, "Verifying");
    }
    if (cb) cb(total_size, total_size, "Verifying");
    free(buf1); free(buf2);
    if (r1 < 0) {
        perror("read");
        return -1;
    }
    return 0;
}
 