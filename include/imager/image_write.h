#ifndef IMAGE_WRITE_H
#define IMAGE_WRITE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include "imager/image_format.h"
#include "imager/progress.h"

int write_image_to_device(const char *image_path, image_format_t format,
                          const char *dev_path, progress_callback_t cb,
                          off_t *bytes_written);

int verify_device_against_image(const char *image_path, image_format_t format,
                                const char *dev_path, off_t total_bytes,
                                progress_callback_t cb);

#ifdef __cplusplus
}
#endif

#endif
