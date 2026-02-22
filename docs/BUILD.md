# Building AvdanOS Imager

## Overview

This project supports both a CLI and a GUI version across Linux, macOS, and Windows.

## Prerequisites

- **GCC compiler** (MinGW-w64 for Windows) or **MSVC** (Visual Studio).
- **CMake 3.10+**.
- **wxWidgets 3.2+** (automatically downloaded via CMake for the GUI version).

## Build Methods

### Using CMake (Recommended for All Platforms)

```powershell
mkdir build
cd build
cmake ..
cmake --build .
```

This will create:
- `imager`: The command-line version.
- `imager-gui`: The graphical user interface version.

### Using Makefile (Linux/macOS CLI Only)

```bash
make
```

## Platform-Specific Notes

### Windows
If using Visual Studio, you can open the project folder directly or use the CMake GUI to generate a `.sln` file.

### Linux
Ensure you have the development headers for your graphics drivers (Mesa) and X11/Wayland if building the GUI.

## Development

```bash
# Debug build (with CMake)
mkdir build-debug
cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```