#ifndef IMAGE_READER_H
#define IMAGE_READER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <stddef.h>
#include "imager/image_format.h"

typedef struct image_reader image_reader_t;

image_reader_t *image_reader_open(const char *path, image_format_t format);

long image_reader_read(image_reader_t *r, void *buf, size_t len);

off_t image_reader_input_pos(const image_reader_t *r);

off_t image_reader_input_size(const image_reader_t *r);

int image_reader_is_compressed(const image_reader_t *r);

void image_reader_close(image_reader_t *r);

int image_reader_format_supported(image_format_t fmt);

#ifdef __cplusplus
}
#endif

#endif
