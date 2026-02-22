#include "imager/drive_list.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#include <winioctl.h>

int list_available_drives(drive_info_t *drives, int max_drives) {
    int count = 0;
    char physical_path[64];

    for (int i = 0; i < 16 && count < max_drives; i++) {
        snprintf(physical_path, sizeof(physical_path), "\\\\.\\PhysicalDrive%d", i);
        
        HANDLE hDevice = CreateFileA(physical_path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hDevice != INVALID_HANDLE_VALUE) {
            DISK_GEOMETRY_EX geometry;
            DWORD bytesReturned;
            
            if (DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &geometry, sizeof(geometry), &bytesReturned, NULL)) {
                drives[count].size = geometry.DiskSize.QuadPart;
                strncpy(drives[count].path, physical_path, sizeof(drives[count].path));
                
                // Get friendly name
                STORAGE_PROPERTY_QUERY query;
                query.PropertyId = StorageDeviceProperty;
                query.QueryType = PropertyStandardQuery;
                
                char buffer[1024];
                if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), buffer, sizeof(buffer), &bytesReturned, NULL)) {
                    STORAGE_DEVICE_DESCRIPTOR *desc = (STORAGE_DEVICE_DESCRIPTOR *)buffer;
                    char vendor[256] = "";
                    char product[256] = "";
                    
                    if (desc->VendorIdOffset != 0) {
                        strncpy(vendor, buffer + desc->VendorIdOffset, sizeof(vendor) - 1);
                        vendor[sizeof(vendor) - 1] = '\0';
                    }
                    if (desc->ProductIdOffset != 0) {
                        strncpy(product, buffer + desc->ProductIdOffset, sizeof(product) - 1);
                        product[sizeof(product) - 1] = '\0';
                    }
                    
                    snprintf(drives[count].name, sizeof(drives[count].name), "%s %s (%.2f GB)", vendor, product, (double)drives[count].size / (1024.0 * 1024.0 * 1024.0));
                    drives[count].is_removable = (desc->BusType == BusTypeUsb || desc->BusType == BusTypeSd || desc->BusType == BusTypeMmc);
                } else {
                    snprintf(drives[count].name, sizeof(drives[count].name), "Physical Drive %d (%.2f GB)", i, (double)drives[count].size / (1024.0 * 1024.0 * 1024.0));
                    drives[count].is_removable = 0;
                }
                
                count++;
            }
            CloseHandle(hDevice);
        }
    }
    return count;
}
#elif __linux__
#include <dirent.h>
#include <unistd.h>

int list_available_drives(drive_info_t *drives, int max_drives) {
    int count = 0;
    DIR *dir = opendir("/sys/block");
    if (!dir) return -1;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && count < max_drives) {
        if (strncmp(entry->d_name, "sd", 2) == 0 || strncmp(entry->d_name, "nvme", 4) == 0) {
            char path[512], buf[256];
            FILE *f;

            snprintf(drives[count].path, sizeof(drives[count].path), "/dev/%s", entry->d_name);
            
            // Size
            snprintf(path, sizeof(path), "/sys/block/%s/size", entry->d_name);
            f = fopen(path, "r");
            if (f) {
                long long sectors;
                if (fscanf(f, "%lld", &sectors) == 1) {
                    drives[count].size = sectors * 512;
                }
                fclose(f);
            }

            // Removable
            snprintf(path, sizeof(path), "/sys/block/%s/removable", entry->d_name);
            f = fopen(path, "r");
            if (f) {
                fscanf(f, "%d", &drives[count].is_removable);
                fclose(f);
            }

            // Model
            snprintf(path, sizeof(path), "/sys/block/%s/device/model", entry->d_name);
            f = fopen(path, "r");
            if (f) {
                fgets(buf, sizeof(buf), f);
                char *nl = strchr(buf, '\n'); if (nl) *nl = '\0';
                snprintf(drives[count].name, sizeof(drives[count].name), "%s (%.2f GB)", buf, (double)drives[count].size / (1024*1024*1024));
                fclose(f);
            } else {
                snprintf(drives[count].name, sizeof(drives[count].name), "%s (%.2f GB)", entry->d_name, (double)drives[count].size / (1024*1024*1024));
            }
            
            count++;
        }
    }
    closedir(dir);
    return count;
}
#elif __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IOBlockStorageDevice.h>

int list_available_drives(drive_info_t *drives, int max_drives) {
    // macOS implementation is very verbose with IOKit. 
    // For this prototype, let's use a simpler heuristic or leave as placeholder.
    // In a real app, we'd use DADiskSession or IOKit.
    return 0; 
}
#else
int list_available_drives(drive_info_t *drives, int max_drives) {
    return 0;
}
#endif
