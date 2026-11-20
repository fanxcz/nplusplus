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
the far jump into 64-bit code.

A GDB remote-stub trace (QEMU `-s -S`, stop at the trampoline and at
`mov eax, cr0`, then single-step) established:

- the PVH note is found and the 32-bit stub executes;
- the identity page tables are written correctly and are readable before
  `CR0.PG` is enabled (`0x...007` / `0x...007` / 512 x `0x83 + i*0x1000`);
- `RAX = CR3 value` is present in the register file right before the
  `mov cr3, eax` / `mov cr0, eax` pair;
- the very first instruction fetch after `mov cr0, eax` fails, and GDB
  memory reads start returning `E14` (EFAULT) for **every** address.

Tried without success: 4-KiB / 2-MiB / 1-GiB pages, tables at several
physical addresses (inside and outside the ELF's 64-MiB BSS), `CR3` before
and after `CR0.PG`, `invd`, dual PAE+long-mode table layouts, `ljmp` and
`retf` far transfers, `microvm` machine type. The fault is therefore in the
QEMU TCG paging-mode transition itself, not in the N++ program or ELF.

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
