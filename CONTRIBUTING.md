# Contributing

Thanks for taking an interest in the imager. This document explains how to
get a change merged with a minimum of friction.

## Before you start

For anything bigger than a typo fix or a small bug fix, please open an
issue first and describe what you want to change and why. It saves both of
us time if we agree on the approach before you write the code. Small,
focused changes get reviewed quickly; thousand-line drive-by rewrites
usually don't get reviewed at all.

## Building

See [docs/BUILD.md](docs/BUILD.md). The short version:

```sh
mkdir build && cd build
cmake ..
cmake --build .
```

The first build compiles wxWidgets and takes a few minutes. If you are only
touching the CLI, `make` in the repository root is much faster.

## Code style

There is no formatter config yet, so match the surrounding code. In
practice that means:

- The core is C99. C++ is only used in `src/gui_main.cpp`. Don't introduce
  C++ into `src/core/` or `src/utils/` otherwise I will personally find you and do bad stuff to you.
- Four-space indentation, braces on the same line.
- Every function that can fail returns `0` on success and `-1` on error,
  and prints a useful message to stderr at the point of failure.
- The core has no hard third-party dependencies. Decompression codecs are
  optional and guarded by `HAVE_ZLIB`, `HAVE_LZMA`, `HAVE_BZIP2` and
  `HAVE_ZSTD`; code must still compile when none of them are defined.
- Platform-specific code lives behind `#if defined(_WIN32)` /
  `#elif defined(__APPLE__)` / `#else` blocks, in that order. If you add a
  platform path, make sure the other two still build.
- Headers that are included from the GUI need `extern "C"` guards.

## Testing your changes

Do not test destructive changes against a disk you care about. On Linux, a
loop device behaves like a real block device and costs nothing:

```sh
truncate -s 2G /tmp/testdisk.img
sudo losetup -fP /tmp/testdisk.img
losetup -l                      # note the /dev/loopN it was assigned
sudo ./imager something.iso /dev/loopN
```

A cheap sacrificial USB stick is the next best thing, and the only way to
test the Windows volume-locking path properly.

At a minimum, before opening a merge request:

- Build with CMake without warnings on your platform.
- Flash a hybrid ISO and check that verification passes (I'll supply one later on, alongside making a CI/CD pipeline).
- If you touched format detection or the streaming reader, also test one
  compressed image and one plain `.img` file.

## Submitting changes

1. Fork the repository and create a branch off the default branch.
2. Keep each merge request to one topic. A bug fix and an unrelated
   refactor should be two MRs.
3. Write commit messages that explain *why*, not just *what*. First line
   under 72 characters, imperative mood ("Fix off-by-one in GPT probe",
   not "Fixed" or "Fixes").
4. Open a merge request against the default branch. Direct pushes to the
   default branch are not accepted.
5. If review feedback comes in, push follow-up commits; don't force-push
   over the discussion until the review is done.

## Reporting bugs

A good bug report includes:

- Your OS and version, and whether you used the CLI or the GUI.
- The exact command you ran and its complete output.
- What kind of image you were flashing (distro, whether it was
  compressed, roughly how large).
- The target device type (USB 2/3 stick, SD card via reader, etc.).

"Verification failed" reports without the above are usually a dying USB
stick, and there is not much we can do with them.
