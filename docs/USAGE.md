# AvdanOS Imager Usage Guide

## Overview

AvdanOS Imager writes ISO images to USB devices. It includes:
- **CLI version (`imager`)**: For fast, command-line usage.
- **GUI version (`imager-gui`)**: For a user-friendly drag-and-drop experience.

## Basic Usage (CLI)

### Linux/macOS
```bash
sudo ./imager <iso_file> <usb_device>
```

### Windows (Administrator)
```powershell
.\imager.exe <iso_file> \\.\PhysicalDriveX
```

## GUI Usage

1. Launch `imager-gui`.
2. Drag and drop your `.iso` file onto the window.
3. Enter the target device path.
4. Click **FLASH!**.

## Finding Your USB Device

### Windows
Run `wmic diskdrive list brief` or use **Disk Management** to find the `PhysicalDrive` number.

### Linux
```bash
lsblk
```

### macOS
```bash
diskutil list
```

## Device Paths

| OS | Example Path |
|----|--------------|
| Windows | `\\.\PhysicalDrive1` |
| Linux | `/dev/sdX` |
| macOS | `/dev/diskN` |

## Safety Warnings

⚠️ **IMPORTANT**: This tool will **completely erase** all data on the target device!

- Always double-check the device path.
- Ensure you have backups.
- Run as Administrator/Root.

## Troubleshooting

### Permission Denied
Ensure you are running the terminal (or the GUI app) as **Administrator** (Windows) or using **sudo** (Linux/macOS).
```bash
sudo ./imager ubuntu.iso /dev/sdX
```

### Device Not Found
- Ensure the USB device is properly connected.
- Check if the device is mounted; some OSs block raw access to mounted drives.
- On Windows, ensure you used the `\\.\PhysicalDriveX` format or use the dropdown in the GUI.

### Verification Failed
- Try writing again.
- Check if the USB device has sufficient space.
- Ensure the device is not defective. Both the CLI and GUI always perform a verification pass after writing.

## Advanced Usage

### Building from Source
See [BUILD.md](BUILD.md) for build instructions.

### Installation (Linux/macOS)
```bash
sudo cp imager /usr/local/bin/
```

Then use from anywhere:
```bash
sudo imager ubuntu.iso /dev/sdX
```

## Future GUI Features

While the initial GUI is functional, I plan to add:
- Automated `.iso` downloading.
- Advanced partitioning options.
- User-friendly interface similar to Balena Etcher.
