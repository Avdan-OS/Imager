#ifndef DEVICE_LOCK_H
#define DEVICE_LOCK_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct device_lock device_lock_t;

#define DEVICE_LOCK_UNMOUNT_ONLY 0
#define DEVICE_LOCK_EXCLUSIVE    1

device_lock_t *device_lock_acquire(const char *dev_path, int exclusive);

void device_lock_release(device_lock_t *lock);

#ifdef __cplusplus
}
#endif

#endif
