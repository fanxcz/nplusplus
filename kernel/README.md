# N++ kernel target (work in progress)

This directory contains a **freestanding experiment** built on the builtin
native backend's `kernel` target. It is not a finished operating system.

## What works

- `npp build kernel/hello.npp -o hello.elf --target kernel --backend native`
  produces a 32-bit-entry ELF64 image with:
  - a Multiboot2 header (for GRUB-style loaders),
  - a Xen PVH `PHYS32_ENTRY` note (for QEMU's `-kernel`),
  - a hand-encoded 32→64-bit trampoline (identity paging, long-mode GDT),
  - no imports, no runtime syscalls, no host toolchain needed.
- On QEMU the PVH loader reaches the trampoline; serial port output confirms
  the 32-bit stub executes and the page-table words are written correctly.

## Known limitation

Booting under `qemu-system-x86_64 -kernel hello.elf` currently ends in a
page fault (`#PF`, error code 0) immediately after `CR0.PG` is set, before
the far jump into 64-bit code. Diagnostics performed on the QEMU monitor
and via COM1 readback confirmed:

- the PVH note is found and the 32-bit stub executes;
- PML4/PDPT/PD entries are written correctly (`0x...007`, `0x...007`, `0x83`);
- `CR3`, `CR4.PAE`, `EFER.LME` and `CR0.PG` all hold the expected values;
- swapping `CR3`/`CR0` ordering, using 2-MiB / 1-GiB / 4-KiB pages, moving
  the tables to several physical addresses, and `invd` did not help.

The remaining fault is therefore in the QEMU TCG paging-mode transition,
not in the N++ program or the ELF layout.

Because of this, `hello.npp` does **not** yet print to VGA/COM1 under QEMU.

## Next steps

1. Replace the hand-encoded trampoline with a small clang-assembled 32-bit
   stub (assembler handles descriptor encodings), keeping the jump target as
   a patchable constant.
2. Re-check the EFER.LME / CR0.PG / far-jump ordering against QEMU's `pvh.bin`.
3. Add a CTest smoke test that boots the image headlessly and checks COM1.

## Files

- `hello.npp` — minimal kernel written in N++ (VGA output + PS/2 key echo).
- `run-kernel.sh`, `run-kernel.ps1` — QEMU launch helpers.
