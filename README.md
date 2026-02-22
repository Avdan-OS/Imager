# AvdanOS Imager

This is a Balena Etcher alternative written in C and C++.

## To-Do List:

```
Legend:
🚧 = We are working on it!
❌ = Implementation hasn't started yet.
✔️ = We have a working implementation.
❔ = May be implemented in the future.
```

`✔️` Core ISO Imaging Engine (CLI)

`🚧` GUI Interface (Initial Implementation)

`🚧` Cross-Platform GUI Framework (wxWidgets)

## Current Status

The project now supports both a **working CLI** and a **native C++ GUI**:

### CLI Features:
- ✅ ISO to USB writing
- ✅ Progress tracking
- ✅ Write verification (Bit-for-bit)
- ✅ Cross-platform support (Linux, macOS, and Windows)

### GUI Features (Initial Implementation):
- ✅ **Native Windows Vibe**: Uses wxWidgets for a Rufus-like aesthetic.
- ✅ **Automated Drive Selection**: Lists friendly drive names and capacities.
- ✅ **Write Verification**: Integrated bit-for-bit verification after flashing.
- ✅ **Safety Filtering**: Filters out internal drives by default to prevent data loss.
- ✅ **High-Capacity Support**: Correctly handles drives larger than 500GB.
- ✅ **Multi-threaded**: UI remains responsive during flashing and verification.

> [!NOTE]
> The current GUI is an initial functional implementation. The final design and layout are subject to further refinement.

## Project Structure

```
├── include/imager/
│   ├── progress.h
│   ├── utils.h
│   ├── drive_list.h
│   ├── iso_operations.h
│   └── imager.h
├── src/
│   ├── core/
│   │   ├── progress.c
│   │   └── iso_operations.c
│   ├── utils/
│   │   ├── utils.c
│   │   └── drive_list.c
│   ├── main.c        (CLI Entry)
│   └── gui_main.cpp  (GUI Entry)
├── docs/
│   ├── BUILD.md
│   └── USAGE.md
├── Makefile
├── CMakeLists.txt
└── README.md
```

## Getting Started

### Prerequisites
- **GCC compiler** (MinGW-w64 for Windows) or **MSVC** (Visual Studio).
- **CMake 3.10+**.
- **wxWidgets 3.2+** (Automatically downloaded via CMake).

### Build (CMake - Recommended)
```bash
mkdir build && cd build
cmake ..
cmake --build .
```
This will generate two executables:
- `imager`: The command-line version.
- `imager-gui`: The native graphical version.

### Usage (GUI)
Simply run `imager-gui` as Administrator/Root, select your ISO, and choose your target USB drive from the dropdown.

### Usage (CLI)
```bash
# Linux/macOS
sudo ./imager <iso_file> <usb_device>

# Windows (Run as Administrator)
.\imager.exe <iso_file> \\.\PhysicalDriveX
```

## Safety
- **You must run as root/admin to access raw devices.**
- **All data on the target device will be destroyed!**
- Double-check your device selection before clicking START.
- The app filters out internal drives by default for safety.

## Documentation
- **[BUILD.md](docs/BUILD.md)** - Detailed build instructions
- **[USAGE.md](docs/USAGE.md)** - Complete usage guide and troubleshooting

## License
GPLv3


