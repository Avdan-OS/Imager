# AvdanOS Imager Usage Guide

## Overview

AvdanOS Imager is a tool for writing ISO images to USB devices. It features progress tracking and verification to ensure data integrity. This is the CLI version of the planned GUI application.

## Current Status

This is currently a **command-line only** implementation. The GUI version is planned for future development.

## Basic Usage

```bash
sudo ./imager <iso_file> <usb_device>
```

### Example

```bash
sudo ./imager ubuntu-22.04.iso /dev/sdX
```

## Safety Warnings

⚠️ **IMPORTANT**: This tool will **completely erase** all data on the target device!

- Always double-check the device path
- Ensure you have backups of important data
- The program will prompt for confirmation before proceeding

## Finding Your USB Device

### Linux
```bash
lsblk
# or
sudo fdisk -l
```

### macOS
```bash
diskutil list
```

Look for your USB device (usually appears as `/dev/disk2` or similar).

## Device Paths

| OS | Example Path |
|----|--------------|
| Linux | `/dev/sdX` (where X is a letter) |
| macOS | `/dev/diskN` (where N is a number) |


## Troubleshooting

### Permission Denied
```bash
sudo ./imager ubuntu.iso /dev/sdX
```

### Device Not Found
- Ensure the USB device is properly connected
- Check device path with `lsblk` or `diskutil list`
- Make sure the device is not mounted

### Verification Failed
- Try writing again
- Check if the USB device has sufficient space
- Ensure the device is not defective

## Advanced Usage

### Building from Source
See [BUILD.md](BUILD.md) for build instructions.

### Installation
```bash
sudo cp imager /usr/local/bin/
```

Then use from anywhere:
```bash
sudo imager ubuntu.iso /dev/sdX
```

## Future GUI Features

When the GUI is implemented, you can expect:

- Drag-and-drop ISO file selection
- Visual device selection
- Progress bars and status updates
- Cross-platform compatibility
- User-friendly interface similar to Balena Etcher 