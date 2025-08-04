#ifndef ISO_OPERATIONS_H
#define ISO_OPERATIONS_H

#include <sys/types.h>

int open_iso_file(const char *iso_path, off_t *iso_size);
int open_device_file(const char *dev_path, int write_mode);
int write_iso_to_device(const char *iso_path, const char *dev_path);
int verify_device_against_iso(const char *iso_path, const char *dev_path, off_t iso_size);

#endif