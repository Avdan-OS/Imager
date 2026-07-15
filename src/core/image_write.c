#include "imager/image_write.h"
#include "imager/image_reader.h"
#include "imager/iso_operations.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#define dev_write _write
#define dev_read _read
#define dev_close _close
#define dev_flush _commit
#else
#include <unistd.h>
#define dev_write write
#define dev_read read
#define dev_close close
#define dev_flush fsync
#endif

#define BLOCK 65536
#define SECTOR 512

int write_image_to_device(const char *image_path, image_format_t format,
                          const char *dev_path, progress_callback_t cb,
                          off_t *bytes_written) {
    image_reader_t *reader = image_reader_open(image_path, format);
    if (reader == NULL) {
        return -1;
    }

    int dev_fd = open_device_file(dev_path, 1);
    if (dev_fd < 0) {
        image_reader_close(reader);
        return -1;
    }

    unsigned char *buf = (unsigned char *)malloc(BLOCK);
    if (buf == NULL) {
        perror("malloc");
        image_reader_close(reader);
        dev_close(dev_fd);
        return -1;
    }

    printf("\nWriting image to device%s...\n",
           image_reader_is_compressed(reader) ? " (streaming decompression)" : "");

    off_t written = 0;
    int ok = 1;

    for (;;) {
        size_t have = 0;
        while (have < BLOCK) {
            long n = image_reader_read(reader, buf + have, BLOCK - have);
            if (n < 0) {
                ok = 0;
                break;
            }
            if (n == 0) break;
            have += (size_t)n;
        }
        if (!ok || have == 0) break;

        size_t padded = (have + (SECTOR - 1)) & ~((size_t)SECTOR - 1);
        if (padded > have) {
            memset(buf + have, 0, padded - have);
        }
        if ((size_t)dev_write(dev_fd, buf, (unsigned int)padded) != padded) {
            perror("write device");
            ok = 0;
            break;
        }
        written += (off_t)have;
        if (cb) cb(image_reader_input_pos(reader), image_reader_input_size(reader), "Writing ");
        if (have < BLOCK) break;
    }

    if (ok) {
        if (cb) cb(image_reader_input_size(reader), image_reader_input_size(reader), "Writing ");
        dev_flush(dev_fd);
        printf("\nWrite complete.\n\n");
        if (bytes_written) *bytes_written = written;
    } else {
        fprintf(stderr, "\nWrite failed.\n");
    }

    free(buf);
    image_reader_close(reader);
    dev_close(dev_fd);
    return ok ? 0 : -1;
}

int verify_device_against_image(const char *image_path, image_format_t format,
                                const char *dev_path, off_t total_bytes,
                                progress_callback_t cb) {
    image_reader_t *reader = image_reader_open(image_path, format);
    if (reader == NULL) {
        return -1;
    }

    int dev_fd = open_device_file(dev_path, 0);
    if (dev_fd < 0) {
        image_reader_close(reader);
        return -1;
    }

    unsigned char *src = (unsigned char *)malloc(BLOCK);
    unsigned char *dst = (unsigned char *)malloc(BLOCK);
    if (src == NULL || dst == NULL) {
        perror("malloc");
        free(src);
        free(dst);
        image_reader_close(reader);
        dev_close(dev_fd);
        return -1;
    }

    printf("Verifying written data...\n");

    off_t remaining = total_bytes;
    int ok = 1;

    while (remaining > 0 && ok) {
        size_t want = remaining < (off_t)BLOCK ? (size_t)remaining : BLOCK;
        size_t have = 0;
        while (have < want) {
            long n = image_reader_read(reader, src + have, want - have);
            if (n < 0) {
                ok = 0;
                break;
            }
            if (n == 0) break;
            have += (size_t)n;
        }
        if (!ok) break;
        if (have == 0) {
            fprintf(stderr, "Verification failed: source ended early.\n");
            ok = 0;
            break;
        }

        size_t padded = (have + (SECTOR - 1)) & ~((size_t)SECTOR - 1);
        size_t got = 0;
        while (got < padded) {
            int n = dev_read(dev_fd, dst + got, (unsigned int)(padded - got));
            if (n <= 0) {
                fprintf(stderr, "Verification failed: device shorter than image.\n");
                ok = 0;
                break;
            }
            got += (size_t)n;
        }
        if (!ok) break;

        if (memcmp(src, dst, have) != 0) {
            fprintf(stderr, "Verification failed: data mismatch.\n");
            ok = 0;
            break;
        }
        remaining -= (off_t)have;
        if (cb) cb(image_reader_input_pos(reader), image_reader_input_size(reader), "Verifying");
    }

    if (ok) {
        if (cb) cb(image_reader_input_size(reader), image_reader_input_size(reader), "Verifying");
        printf("\nVerification successful!\n");
    }

    free(src);
    free(dst);
    image_reader_close(reader);
    dev_close(dev_fd);
    return ok ? 0 : -1;
}
