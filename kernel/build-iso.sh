#!/usr/bin/env bash
# Build a bootable GRUB2 ISO containing the N++ kernel.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
npp="${NPP:-$root/build/npp}"
out="${1:-$root/kernel/npp-kernel.iso}"
stage="$(mktemp -d)"
trap 'rm -rf "$stage"' EXIT
mkdir -p "$stage/boot/grub"
"$npp" build "$root/kernel/hello.npp" -o "$stage/kernel.elf" \
  --target kernel --backend native
cat > "$stage/boot/grub/grub.cfg" <<'CFG'
serial --unit=0 --speed=115200
terminal_input serial
terminal_output serial
set timeout=0
menuentry "N++ Kernel" {
  multiboot2 /kernel.elf
  boot
}
CFG
grub-mkrescue -o "$out" "$stage"
echo "ISO: $out"
