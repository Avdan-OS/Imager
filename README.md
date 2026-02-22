# AvdanOS Imager

This is a Balena Etcher alternative written in C.

## To-Do List:

```
Legend:
🚧 = We are working on it!
❌ = Implementation hasn't started yet.
✔️ = We have a working implementation.
❔ = May be implemented in the future.
```

`✔️` Core ISO Imaging Engine (CLI)

`❌` GUI Interface

`❌` Cross-Platform GUI Framework

## Current Status

The project currently has a **working CLI implementation** with:
- ✅ ISO to USB writing
- ✅ Progress tracking
- ✅ Write verification
- ✅ Cross-platform support (Linux, macOS, and Windows)

The **GUI is planned** but not yet implemented.

## Project Structure

```
├── include/imager/
│   ├── progress.h
│   ├── utils.h
│   ├── iso_operations.h
│   └── imager.h
├── src/
│   ├── core/
│   │   ├── progress.c
│   │   └── iso_operations.c
│   ├── utils/
│   │   └── utils.c
│   └── main.c
├── docs/
│   ├── BUILD.md
│   └── USAGE.md
├── Makefile
├── CMakeLists.txt
└── README.md
```

## Getting Started

### Prerequisites
- GCC compiler (or compatible C compiler)
- Make (for Makefile builds)
- CMake 3.10+ (for CMake builds)

### Build
```bash
make
```

### Usage (CLI)
```bash
# Linux/macOS
sudo ./imager <iso_file> <usb_device>

# Windows (Run as Administrator)
.\imager.exe <iso_file> \\.\PhysicalDriveX
```

Example (Linux):
```bash
sudo ./imager ubuntu-22.04.iso /dev/sdX
```

Example (Windows):
```bash
.\imager.exe ubuntu-22.04.iso \\.\PhysicalDrive1
```

## Documentation

- **[BUILD.md](docs/BUILD.md)** - Detailed build instructions
- **[USAGE.md](docs/USAGE.md)** - Complete usage guide and troubleshooting

## Safety
- **You must run as root/admin to access raw devices.**
- **All data on the target device will be destroyed!**
- Double-check your device path before proceeding
- The program will prompt for confirmation before writing

## Build Systems

### Makefile (Recommended)
```bash
make
make clean
```

### CMake
```bash
mkdir build && cd build
cmake ..
make
```

## Contributing

Please see the [contributing guidelines](CONTRIBUTING.md) for more info.

## License
GPLv3


