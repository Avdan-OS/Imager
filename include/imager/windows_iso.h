#ifndef WINDOWS_ISO_H
#define WINDOWS_ISO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "imager/progress.h"

int write_iso_extracted(const char *iso_path, const char *dev_path,
                        progress_callback_t cb);

#ifdef __cplusplus
}
#endif

#endif
