# Building AvdanOS Imager

## Overview

This document covers building the AvdanOS Imager project. Currently, only the CLI version is implemented.

## Prerequisites

- GCC compiler (or compatible C compiler)
- Make (for Makefile builds)
- CMake 3.10+ (for CMake builds)

## Build Methods

### Using Makefile (Recommended)

```bash
make
```

This will create the `imager` executable in the project root.

### Using CMake

```bash
mkdir build
cd build
cmake ..
make
```

### Manual Compilation

```bash
gcc -O2 -Wall -I./include -o imager \
    src/main.c \
    src/core/progress.c \
    src/utils/utils.c \
    src/core/iso_operations.c
```

## Cleaning

```bash
make clean
```

Or for CMake builds:

```bash
cd build
make clean
```

## Installation

After building, you can install the binary:

```bash
sudo cp imager /usr/local/bin/
```

## Future GUI Build

When the GUI is implemented, additional dependencies may be required:

- GTK+ (for Linux GUI)
- Cocoa (for macOS GUI)
- Windows API (for Windows GUI)
- Or some cross-platform framework idk yet

## Development

For development, you can use:

```bash
# Debug build
make CFLAGS="-O0 -g -Wall -I./include"

# Or with CMake
mkdir build-debug
cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```