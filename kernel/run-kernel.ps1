# Build and boot the N++ kernel with QEMU, completely headless:
# no window, no console, no terminal output. Serial goes to serial.txt.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$npp = Join-Path $root 'build\Release\npp.exe'
if (-not (Test-Path $npp)) { $npp = Join-Path $root 'build\npp.exe' }
$elf = Join-Path $PSScriptRoot 'hello.elf'
$log = Join-Path $PSScriptRoot 'serial.txt'

& $npp build (Join-Path $PSScriptRoot 'hello.npp') -o $elf --target kernel --backend native
Remove-Item -Force $log -ErrorAction SilentlyContinue

$qemu = 'qemu-system-x86_64'
if (Get-Command $qemu -ErrorAction SilentlyContinue) {
    $exe = (Get-Command $qemu).Source
} else {
    $exe = 'C:\Program Files\qemu\qemu-system-x86_64.exe'
}
Start-Process -FilePath $exe -PassThru -WindowStyle Hidden -ArgumentList @(
    '-kernel', $elf, '-display', 'none', '-serial', "file:$log", '-no-reboot'
) | Out-Null
Write-Output "QEMU запущен скрыто. Serial: $log"
