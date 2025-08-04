#ifndef PROGRESS_H
#define PROGRESS_H

#include <sys/types.h>

void print_progress(off_t done, off_t total, const char *label);
int copy_with_progress(int in_fd, int out_fd, off_t total_size, const char *label);
int verify_with_progress(int iso_fd, int dev_fd, off_t total_size);

#endif