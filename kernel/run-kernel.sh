#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
npp="${NPP:-$root/build/npp}"
"$npp" build "$root/kernel/hello.npp" -o "$root/kernel/hello.elf" \
  --target kernel --backend native
qemu-system-x86_64 -kernel "$root/kernel/hello.elf" -serial stdio -no-reboot
