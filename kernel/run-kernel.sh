#!/usr/bin/env bash
# Build and boot the N++ kernel completely headless.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
npp="${NPP:-$root/build/npp}"
elf="$root/kernel/hello.elf"
"$npp" build "$root/kernel/hello.npp" -o "$elf" --target kernel --backend native
exec qemu-system-x86_64 -kernel "$elf" -display none -serial stdio -no-reboot
