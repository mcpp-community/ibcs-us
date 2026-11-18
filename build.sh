#!/usr/bin/env bash
# ibcs-us BUILT FROM ITS RELEASED SOURCE WITH THE CORRECTIONS OF patches/.
#
#   build.sh <work> <out>
#
# <work> receives the source; <out> receives `bin/ibcs-us' and the files a
# distribution of it carries. The source archive is the upstream's release as it
# was published (upstream/, from
# https://sourceforge.net/projects/ibcs-us/files/ibcs-us-4.3-1/), held to the
# digest below.
#
# The recipe needs a GCC that compiles for i386 and nothing of an i386 system:
# the program is free-standing (`-nostdlib'), so the two things it takes from a
# 32-bit development package are supplied here --- the empty `gnu/stubs-32.h'
# the host's <features.h> asks for, and the one routine of libgcc the objects
# refer to (build/pc-thunk.S).
#
# `-O1': at the source's `-O2', GCC 16 produced a loader that failed in open().
# `-std=gnu17': the source defines `bool'. `-fno-strict-aliasing': the source
# reads kernel structures through other types.
set -euo pipefail
UPSTREAM=4.3
SHA256=c49da199c2b47629caf775657dc3728534e29514196bedb4123fcba610522464

here="$(cd "$(dirname "$0")" && pwd)"
mkdir -p "${1:?work directory}" "${2:?output directory}"
work="$(cd "$1" && pwd)"; out="$(cd "$2" && pwd)"
archive="$here/upstream/ibcs-us-$UPSTREAM.tar.gz"
echo "$SHA256  $archive" | sha256sum -c -

rm -rf "$work/ibcs-us-$UPSTREAM" "$work/stub32"
tar -xzf "$archive" -C "$work"
src="$work/ibcs-us-$UPSTREAM"
for p in "$here"/patches/*.patch; do patch -d "$src" -p1 --no-backup-if-mismatch < "$p"; done

mkdir -p "$work/stub32/gnu"; : > "$work/stub32/gnu/stubs-32.h"
"${CC:-gcc}" -m32 -c -o "$work/pc-thunk.o" "$here/build/pc-thunk.S"
make -C "$src" -j"$(nproc)" CC="${CC:-gcc}" \
  CFLAGS_OPT="-O1 -fno-builtin -ffreestanding -fno-strict-aliasing" \
  CFLAGS_EXTRA="-I$work/stub32 -std=gnu17" CFLAGS_ERROR="" \
  LDFLAGS_LIBS="-nostdlib $work/pc-thunk.o"

mkdir -p "$out/bin" "$out/share/ibcs-us/patches"
cp "$src/build/ibcs-us" "$out/bin/ibcs-us"
strip "$out/bin/ibcs-us"
cp "$src/gpl-2.txt" "$out/share/ibcs-us/COPYING"
cp "$src/README.txt" "$src/ibcs-us.1" "$src/ChangeLog.txt" "$out/share/ibcs-us/"
cp "$here"/patches/*.patch "$out/share/ibcs-us/patches/"
cp "$here/README.md" "$out/share/ibcs-us/README.mirror.md"
