# Usage

## Synopsis

```
imager [--extract] <image> <device>
```

Writing to a raw device requires elevated privileges: use `sudo` on Linux
and macOS, or an Administrator shell on Windows.

## Supported images

| Type | Example | Behavior |
|------|---------|----------|
| Hybrid ISO (ISO9660 + MBR/GPT) | most Linux distros | written raw, boots BIOS/UEFI |
| Raw disk image (MBR or GPT) | `.img` files, Raspberry Pi images | written raw |
| Compressed image | `foo.iso.gz`, `bar.img.xz`, `.bz2`, `.zst` | decompressed on the fly while writing |
| Plain ISO9660 (no partition table) | some old or exotic ISOs | written raw with a warning; may not boot everywhere |
| Windows install ISO (UDF) | `Win11.iso` | use `--extract`; a raw write will not boot |

The imager identifies the file by its contents, not its extension, and
prints what it found before asking for confirmation:

```
Image: fedora.iso
Detected format: Hybrid ISO (ISO9660 + MBR/GPT)
Volume label:    Fedora-WS-Live
Bootable:        yes (El Torito)
```

## What a flash actually does

1. Analyzes the image and warns about anything that won't boot.
2. Asks you to confirm the target device.
3. Unmounts every filesystem on the device and takes an exclusive lock,
   so nothing else can touch it mid-write. If another program is using
   the drive, the imager aborts here instead of corrupting the write.
4. Writes the image (decompressing on the fly if needed).
5. Reads everything back and compares it bit for bit against the source.

There is no way to skip the verification pass, and that is intentional:
USB sticks lie.

## Windows install ISOs (--extract)

Windows ISOs are not hybrid images, so writing them raw produces a stick
that won't boot. Extraction mode does what tools like Rufus do instead:

```sh
sudo ./imager --extract Win11.iso /dev/sdb
```

This partitions the drive (MBR), formats a FAT32 partition, copies the
ISO contents file by file, and, when `sources/install.wim` is larger than
FAT32 allows (4 GiB), splits it into `.swm` parts that Windows Setup
understands.

Extraction mode relies on standard platform tools being present:

- Linux: `parted`, `mkfs.vfat`, `mount`, `tar`; `wimlib-imagex` (package
  `wimtools`) if the wim needs splitting
- macOS: `hdiutil`, `diskutil`, `tar`; `wimlib` from Homebrew for splitting
- Windows: `diskpart`, PowerShell, `robocopy`, `dism` (all ship with
  Windows)

Known limitations: the resulting stick boots on UEFI systems only, and on
Windows the OS cannot format FAT32 partitions larger than 32 GB.

## Finding your device

Linux:

```sh
lsblk -d -o NAME,SIZE,MODEL,TRAN
```

macOS:

```sh
diskutil list external
```

Windows (Administrator PowerShell):

```powershell
Get-Disk
```

| OS | Device path format |
|----|--------------------|
| Linux | `/dev/sdb`, `/dev/nvme1n1` (the whole disk, not `/dev/sdb1`) |
| macOS | `/dev/disk4` |
| Windows | `\\.\PhysicalDrive2` |

Always pass the whole disk, never a partition.

## GUI

Run `imager-gui` as root/Administrator. It lists removable drives by
name and capacity (internal disks are hidden unless you untick "List USB
drives only"), shows the detected image format and volume label as soon
as you pick a file, and offers a "Windows extraction mode" checkbox that
it pre-selects when it detects Windows install media. Progress and
verification behave exactly like the CLI.

## Troubleshooting

**"Permission denied" / "Error opening device"** - you are not root (or
not an Administrator). Elevate and retry.

**"Could not unmount/lock device"** - something is using the drive: a
file manager window, a terminal with its working directory on the stick,
an indexer, another imaging tool. Close it and retry. On Windows,
Explorer windows showing the drive are the usual culprit.

**"This build has no ... support compiled in"** - the image is compressed
with a codec that wasn't available when the imager was built. Either
decompress the file manually (`xz -d`, `gunzip`, ...) or rebuild with the
codec's development package installed; see
[BUILD.md](BUILD.md).

**"Verification failed"** - the data read back from the stick doesn't
match the image. Retry once; if it fails again the stick is almost
certainly counterfeit or worn out. Cheap high-capacity sticks that fail
exactly at the same percentage every time are reporting more capacity
than they physically have.

**Warning about a plain ISO9660 / UDF image** - the image has no
partition table, so a raw write may not produce a bootable drive. For
Windows ISOs, use `--extract`. For anything else, check whether the
vendor provides a `.img` or hybrid ISO variant.

**The drive "shrank" after flashing** - normal. The OS only sees the
partitions defined by the image. Repartition and reformat the stick to
get the full capacity back.
