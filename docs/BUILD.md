# Building

The project builds two binaries: `imager` (CLI) and `imager-gui`
(wxWidgets GUI). CMake is the primary build system; a plain Makefile is
kept around for quick CLI-only builds on Unix.

## Dependencies

Required:

- A C/C++ compiler: GCC, Clang, MSVC, or MinGW-w64
- CMake 3.10 or newer

Optional, for flashing compressed images. CMake detects whichever of these
are installed and enables the matching codec; missing ones are simply
skipped and the imager will tell the user to decompress such files
manually:

- zlib (`.gz`)
- liblzma / xz-utils (`.xz`)
- bzip2 (`.bz2`)
- zstd (`.zst`)

wxWidgets 3.2 is downloaded and built automatically by CMake; you do not
need to install it. This makes the first configure + build take several
minutes. Subsequent builds are fast.

### Debian / Ubuntu

```sh
sudo apt install build-essential cmake \
    zlib1g-dev liblzma-dev libbz2-dev libzstd-dev \
    libgtk-3-dev
```

`libgtk-3-dev` is only needed for the GUI.

### Fedora

```sh
sudo dnf install gcc gcc-c++ cmake \
    zlib-devel xz-devel bzip2-devel libzstd-devel \
    gtk3-devel
```

### macOS

```sh
xcode-select --install
brew install cmake xz zstd
```

zlib and bzip2 ship with the system.

### Windows

Install Visual Studio (with the "Desktop development with C++" workload)
or MinGW-w64, plus CMake. The codec libraries are optional; the easiest
way to get them is vcpkg:

```powershell
vcpkg install zlib liblzma bzip2 zstd
cmake .. -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
```

Without them the imager builds fine but rejects compressed images.

## Building with CMake

```sh
mkdir build && cd build
cmake ..
cmake --build .
```

The configure step prints which codecs were found:

```
-- Imager: streaming decompression enabled: HAVE_ZLIB;HAVE_LZMA;HAVE_BZIP2;HAVE_ZSTD
```

For a debug build:

```sh
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

On Windows you can also open the folder directly in Visual Studio, which
drives CMake for you.

## Building with the Makefile (CLI only, Unix)

```sh
make
```

The Makefile assumes all four codec libraries are installed. If one is
missing on your system, remove its `-DHAVE_*` flag from `CODEC_DEFS` and
the matching `-l` flag from `CODEC_LIBS` at the top of the Makefile.

## Installing

```sh
cmake --install build        # installs imager and imager-gui to bin/
```

or simply copy the binaries somewhere on your PATH:

```sh
sudo cp build/imager /usr/local/bin/
```
