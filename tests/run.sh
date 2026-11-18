#!/usr/bin/env bash
# THE PROBE UNDER AN ibcs-us, under each interface a static ELF is run as.
#
#   run.sh <ibcs-us> [<work>]
#
# Needs a GCC that compiles for i386 (no i386 library is used) and a host that
# lets an unprivileged user make a user namespace: ibcs-us asks for the root
# identity, and has it there.
set -euo pipefail
ibcs="$(cd "$(dirname "${1:?the ibcs-us to run}")" && pwd)/$(basename "$1")"
here="$(cd "$(dirname "$0")" && pwd)"
work="${2:-$(mktemp -d)}"; mkdir -p "$work"; work="$(cd "$work" && pwd)"
cc="${CC:-gcc}"
"$cc" -m32 -O2 -ffreestanding -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables \
  -nostdlib -static -Wl,--build-id=none -s -o "$work/probe" "$here/sc.S" "$here/probe.c"
unshare -Ur true 2>/dev/null || { echo "this host does not let an unprivileged user make a user namespace" >&2; exit 2; }
status=0
for interface in per_uw7 per_solaris per_osr5; do
  echo "== $interface"
  rm -rf "$work/run"; mkdir "$work/run"
  (cd "$work/run" && unshare -Ur "$ibcs" -p "$interface" ../probe) || status=1
done
exit "$status"
