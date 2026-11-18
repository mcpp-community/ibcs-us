# ibcs-us (mirror with corrections)

[ibcs-us](https://ibcs-us.sourceforge.io/) runs a program of the System V i386 kernel
interface (iBCS2: SCO OpenServer 5, UnixWare 7, Solaris x86 and others) on Linux, in user
space: the program executes on the processor, and each call it makes through the call gate
is answered from Linux system calls. It is Russell Stuart's continuation of the kernel's
iBCS module, under the GNU General Public License, version 2 (`LICENSE`).

This repository is not a fork of its source. It holds the corrections a program needed
(`patches/`), the recipe that builds the released source with them (`build.sh`), a probe
that shows each correction (`tests/`), and the workflow that publishes the result. The
packages `ibcs-us` of the xlings index install what is published here.

| | |
|---|---|
| Upstream release | 4.3, `upstream/ibcs-us-4.3.tar.gz` as published, sha256 `c49da199c2b47629caf775657dc3728534e29514196bedb4123fcba610522464` |
| This repository's release | `4.3-<n>`: the upstream release and the n-th state of `patches/` |
| Host | Linux x86_64 (the program is a static ELF32 i386 executable; a 64-bit kernel must run 32-bit programs) |

## Running a program

```sh
unshare -Ur ibcs-us ./program arguments
```

ibcs-us asks for the root identity or `CAP_SYS_RAWIO` before it does anything; the root of
a user namespace satisfies it, so no privilege of the host is needed. The host must let an
unprivileged user make a user namespace (where AppArmor restricts that:
`sysctl kernel.apparmor_restrict_unprivileged_userns=0`).

The interface a program is run under is read from its ELF header (`e_flags` zero is
UnixWare 7), or given: `-p per_uw7`, `-p per_solaris`, `-p per_osr5`. The source registers
no interface of Release 4.0 itself.

A program that replaces its image (`exec`) leaves the emulation: the call is handed to
Linux, which starts the file as its own. That is the upstream's behaviour and is not
changed here.

## The corrections

ibcs-us 4.3 as released did not load an executable without debugging information, list a
directory, shorten a file or change a file's mode. `patches/ibcs-us-4.3-corrections.patch`:

| File | Defect | Seen as | Correction |
|---|---|---|---|
| `binfmt-elf/binfmt_elf.c` | The file's mapping was extended over the zero-filled part of a segment | SIGBUS in an executable whose file ends before that part | anonymous pages |
| `per-svr4/open.c` | `getdents` wrote the build host's `struct dirent` | names read nine bytes late | the interface's layout, stated in the file |
| `ibcs/linux26-compat.c` | A sixty-four bit offset handed to `lseek` as words: its upper half was read as the origin | a query of the position set it to zero; a directory was listed from its start at every call | `_llseek` |
| `ibcs/linux26-compat.c` | A batch of the host's directory entries was read and its first entry handed over | a directory of one entry | every entry; an entry with no room is read again by the next call |
| `ibcs/linux26-compat.c` | A full buffer was answered as the failure of the reading | under UnixWare 7 and Solaris, a directory larger than the caller's buffer was an error | zero, as the kernel's `vfs_readdir` answers |
| `ibcs/linux26-compat.c` | `chmod` and `chown` entered as the `stat` and `access` handlers | `chmod` of any file answered EFAULT | the handler for a path |
| `per-svr4/fcntl.c` | `F_FREESP` answered success and freed nothing | a shortened file kept its length | a truncation, for a range to the end of the file |

`tests/run.sh <ibcs-us>` builds a program with no C library that makes these calls and runs
it under the three interfaces. Under the upstream's released binary it ends at the first
correction; under this repository's it holds in every line.

## Building

```sh
./build.sh work out        # out/bin/ibcs-us
./tests/run.sh out/bin/ibcs-us
```

A GCC that compiles for i386 is needed (`-m32`), and no i386 library: `build.sh` states why.
The build is at `-O1`: at the source's `-O2`, GCC 16 produced a loader that failed in `open`.

## The source

The published archive carries the binary, the upstream's licence, README, manual page and
change log, and the patches. The corresponding source is this repository at the release's tag: the
upstream archive (`upstream/`, also attached to each release), `patches/` and `build.sh`.
