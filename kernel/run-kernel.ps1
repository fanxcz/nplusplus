# Build and boot the N++ kernel in QEMU's own window (SDL).
# No separate console window is spawned; the OS lives in the QEMU window.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$candidates = @(
    (Join-Path $root 'build\Release\npp.exe'),
    (Join-Path $root 'build\npp.exe'),
    (Join-Path $root 'build-audit\Release\npp.exe'),
    (Join-Path $root 'build-vs\Release\npp.exe')
)
$npp = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $npp) {
    $cmd = Get-Command npp -ErrorAction SilentlyContinue
    if ($cmd) { $npp = $cmd.Source }
}
if (-not $npp) {
    throw "npp.exe not found. Build the compiler first, or put npp in PATH."
}
$elf = Join-Path $PSScriptRoot 'hello.elf'
$log = Join-Path $PSScriptRoot 'serial.txt'

# A previous headless QEMU keeps hello.elf mapped, which blocks rebuilds.
# Stop only the instances that use our image.
Get-CimInstance Win32_Process -Filter "Name = 'qemu-system-x86_64.exe'" -ErrorAction SilentlyContinue |
    Where-Object { $_.CommandLine -and $_.CommandLine -like '*hello.elf*' } |
    ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }
Start-Sleep -Milliseconds 300

& $npp build (Join-Path $PSScriptRoot 'hello.npp') -o $elf --target kernel --backend native
if ($LASTEXITCODE -ne 0) { throw "kernel build failed" }
Remove-Item -Force $log -ErrorAction SilentlyContinue

$qemu = 'qemu-system-x86_64'
if (Get-Command $qemu -ErrorAction SilentlyContinue) {
    $exe = (Get-Command $qemu).Source
} else {
    $exe = 'C:\Program Files\qemu\qemu-system-x86_64.exe'
}
$proc = Start-Process -FilePath $exe -PassThru -ArgumentList @(
    '-kernel', $elf, '-display', 'sdl', '-serial', "file:$log", '-no-reboot'
)
Write-Output "QEMU PID: $($proc.Id) - QEMU window with the OS screen"

# QEMU needs a moment to boot before the serial file is created.
$deadline = (Get-Date).AddSeconds(15)
while ((Get-Date) -lt $deadline -and -not (Test-Path $log)) {
    Start-Sleep -Milliseconds 300
}
if (-not (Test-Path $log)) {
    Write-Output "serial.txt not created yet - check the qemu-system-x86_64.exe path."
} else {
    $deadline = (Get-Date).AddSeconds(10)
    while ((Get-Date) -lt $deadline -and -not (Select-String -Path $log -Pattern 'N\+\+ KERNEL' -Quiet -ErrorAction SilentlyContinue)) {
        Start-Sleep -Milliseconds 300
    }
    Write-Output '=== serial ==='
    Get-Content $log | Select-Object -First 20
}
