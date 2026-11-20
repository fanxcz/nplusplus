# N++ kernel target (freestanding, working)

A freestanding kernel written in N++ and built with the builtin `kernel`
target. It boots in x86-64 long mode under QEMU, with no host OS, no libc,
no syscalls and no external assembler/linker.

## Build and run (headless)

```sh
npp build hello.npp -o hello.elf --target kernel --backend native
qemu-system-x86_64 -kernel hello.elf -display none -serial stdio
```

On Windows, `run-kernel.ps1` does the same with no window and no console
output on screen (QEMU is launched hidden; output goes to `serial.txt`).

## Boot via GRUB2 (Multiboot2)

The image also carries a Multiboot2 header. To boot it through GRUB2:

1. Put `hello.elf` next to `boot/grub/grub.cfg` with:
   ```
   serial --unit=0 --speed=115200
   terminal_input serial
   terminal_output serial
   set timeout=0
   menuentry "N++ Kernel" {
     multiboot2 /kernel.elf
     boot
   }
   ```
2. Build an ISO: `grub-mkrescue -o npp-kernel.iso <dir>`
3. Boot: `qemu-system-x86_64 -cdrom npp-kernel.iso -display none -serial stdio`

`build-iso.sh` automates steps 1–2 on systems with GRUB2 installed.

## How it works

- `writeKernelELF` emits an ELF64 at `0x101000` with a Multiboot2 header,
  a Xen PVH note, and a null section-header table (required by GRUB).
- `e_entry` points at a 32-bit trampoline. Both QEMU's PVH loader and
  GRUB's Multiboot2 loader hand off in 32-bit protected mode; the trampoline
  builds identity page tables at `0x200000`, a long-mode GDT at `0x203000`,
  enables PAE/LME/PG and far-jumps into the 64-bit `_start`.
- `_start` initialises the bump arena from the ELF image, calls `main`, then
  halts. `print()` writes to the VGA text buffer and mirrors to COM1;
  `read_key()` polls the PS/2 controller (press ESC to stop).

## Verified

- Direct PVH boot: `qemu-system-x86_64 -kernel hello.elf` — banner, tick
  counter and keyboard prompt appear on COM1.
- GRUB2 Multiboot2 boot: same output over serial.
- `ctest`: 41/41 tests pass on the regular build.
