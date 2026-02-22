#ifndef PROGRESS_H
#define PROGRESS_H

#include <sys/types.h>

typedef void (*progress_callback_t)(off_t done, off_t total, const char *label);

void print_progress_cli(off_t done, off_t total, const char *label);
int copy_with_progress(int in_fd, int out_fd, off_t total_size, const char *label, progress_callback_t cb);
int verify_with_progress(int iso_fd, int dev_fd, off_t total_size, progress_callback_t cb);

#endif