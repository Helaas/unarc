# unarc

Command-line unpacker for FreeArc archives (`"ArC\1"` header), the format many installers use for their `.bin` data files. `file` reports these as `FreeArc archive <http://freearc.org>`.

This is a fork of [xredor/unarc](https://github.com/xredor/unarc), which is based on the FreeArc 0.60 sources ([andyvand/freearc_src](https://github.com/andyvand/freearc_src)). The fork adds macOS support and fixes for archives made by newer FreeArc versions.

## Changes in this fork

- **macOS.** Builds with the stock Xcode command line tools via `make`. Tested on Apple Silicon; an Intel (`x86_64`) build compiles but hasn't been run. Memory size and CPU count come from `sysctl`/`sysconf`, and the Makefile skips `librt` on Darwin.
- **8 MB stacks for worker threads.** macOS gives secondary threads only 512 KB, and the 256 KB on-stack error buffer overflowed it. Extraction (`x`) crashed even though testing (`t`) worked.
- **FreeArc 0.666 `4x4` method.** Decodes `4x4:...` blocks such as `rep:96mb+4x4:tor:16mb:c3`, which installers built with FreeArc 0.666 often use. The stream is a version word followed by independent `origsize, compsize, data` blocks, each decoded from memory with the inner method.
- **PPMd on 64-bit.** The model stores its internal pointers as 32-bit offsets, so 64-bit builds keep the original 12-byte memory units. They decode archives made by 32-bit FreeArc correctly, including when the model fills its memory limit. Verified: the 64-bit encoder output is byte-identical to the original code built for i386 in restart, cut-off and freeze modes. This also fixes free-block merging, which never ran on 64-bit.
- **Other 64-bit fixes** in MM/TTA, GRZip LZP and the PPMd types, which assumed `long` is 32 bits.
- **Dict:** a clean end of input now ends the stream instead of being reported as an error.
- **UTF-8 sources.** The FreeArc sources were CP1251 (Russian comments) plus a few CP866 files, which modern editors silently corrupt on save.

## Usage

```
Usage: unarc command [options] archive[.arc] [filenames...]
Available commands:
  l - display archive listing
  v - display verbose archive listing
  e - extract files into current directory
  x - extract files with pathnames
  t - test archive integrity
Available options:
  -dp{Path}   - set destination path
  -w{Path}    - set temporary files directory
  -o+         - overwrite existing files
  -o-         - don't overwrite existing files
  --noarcext  - don't add default extension to archive name
  --          - no more options
```

Examples:

```sh
unarc l data.bin                        # list contents
unarc t data.bin                        # test integrity (checks every file's CRC)
unarc x -dp"$HOME/Games/Foo" data.bin   # extract with paths into a folder
```

## Building

- macOS and 64-bit Linux (tested with Apple clang 21 and gcc 14): run `make` (or `make -j N`). The result is a single `unarc` binary; copy it to a folder on your `PATH`.
- 32-bit platforms: first remove `-DFREEARC_64BIT` from `DEFINES` in the `Makefile`.

The program only decompresses; there is no packer.

## Background

From the original project:

- Nothing found in Download category in https://freearc.org
- related repositories seems dead
- Old Haskell stuff not compiled on my new The Glorious Glasgow Haskell Compilation System, version 8.4.4 and since i don't know Haskell i can't fix it
- Installer, GUI, all this is not necessary
