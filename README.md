# AvdanOS Imager

A small, fast tool for writing OS images to USB drives. Think of it as a
no-nonsense alternative to Balena Etcher or Rufus. Plain C for the core, a native
wxWidgets front end instead of a bundled browser engine, and no telemetry.

Runs on Linux, macOS and Windows.

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

## Features

- Raw image writing with a mandatory bit-for-bit verification pass.
- Knows what it is flashing. Hybrid ISOs, plain ISO9660, UDF and raw
  MBR/GPT disk images are detected and validated before a single byte is
  written. Non-bootable combinations produce a warning up front, not a
  dead USB stick twenty minutes later.
- Flashes compressed images (`.gz`, `.xz`, `.bz2`, `.zst`) directly.
  Decompression happens on the fly; no temp files, no manual unpacking.
- Extraction mode for Windows install ISOs (`--extract`): partitions the
  drive, formats FAT32, copies the files and splits `install.wim` when it
  exceeds the FAT32 4 GiB limit.
- Unmounts and locks the target device before writing, so a mounted
  filesystem can't corrupt the write halfway through.
- Two front ends: `imager` for the terminal, `imager-gui` for everyone else.

## Building

You need a C/C++ compiler and CMake 3.10 or newer. The optional
decompression codecs (zlib, liblzma, bzip2, zstd) are picked up
automatically if the development packages are installed.

```sh
mkdir build && cd build
cmake ..
cmake --build .
```

This produces both `imager` and `imager-gui`. wxWidgets is downloaded and
built automatically for the GUI, so the first build takes a while. See
[docs/BUILD.md](docs/BUILD.md) for per-platform details and the
Makefile-only CLI build.

## Usage

Writing an image needs raw device access, so run as root (or from an
Administrator shell on Windows):

```sh
# Linux
sudo ./imager fedora.iso /dev/sdb

# macOS
sudo ./imager archlinux.img.xz /dev/disk4

# Windows (Administrator)
imager.exe ubuntu.iso \\.\PhysicalDrive2
```

Compressed images are handled transparently. For Windows install ISOs,
use extraction mode instead of a raw write:

```sh
sudo ./imager --extract Win11.iso /dev/sdb
```

Or launch `imager-gui`, pick the image and the drive, and press START.
The GUI shows the detected format before you commit to anything.

The full guide, including how to find your device path and what every
warning means, is in [docs/USAGE.md](docs/USAGE.md).

## Safety

Writing an image destroys everything on the target drive. There is no
undo. The imager tries hard to protect you - it filters out internal
disks in the GUI by default, asks for confirmation, and refuses to write
while the device is in use - but the last line of defense is you reading
the device path twice.

## Project layout

```
include/imager/     public headers
src/core/           format detection, streaming reader, write/verify,
                    extraction mode, device locking, progress
src/utils/          drive enumeration, CLI helpers
src/main.c          CLI entry point
src/gui_main.cpp    wxWidgets GUI
docs/               build and usage documentation
```

## Contributing

Bug reports and merge requests are welcome. Please read
[CONTRIBUTING.md](CONTRIBUTING.md) first, especially the part about
testing against loop devices instead of your own SSD.

## License

GPLv3. See [LICENSE](LICENSE).
