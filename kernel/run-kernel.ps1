# Build and (attempt to) boot the N++ kernel image in QEMU.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$npp = Join-Path $root 'build\Release\npp.exe'
if (-not (Test-Path $npp)) { $npp = Join-Path $root 'build\npp.exe' }
& $npp build (Join-Path $PSScriptRoot 'hello.npp') -o (Join-Path $PSScriptRoot 'hello.elf') `
    --target kernel --backend native
qemu-system-x86_64 -kernel (Join-Path $PSScriptRoot 'hello.elf') `
    -serial stdio -no-reboot
