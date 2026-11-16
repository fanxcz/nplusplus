# Register .npp file association + icon for the current user (HKCU, no admin).
#
#   powershell -ExecutionPolicy Bypass -File scripts\set-file-icon.ps1
#   powershell -ExecutionPolicy Bypass -File scripts\set-file-icon.ps1 -NppExe D:\path\to\npp.exe
#   powershell -ExecutionPolicy Bypass -File scripts\set-file-icon.ps1 -Unregister
#
# -NppExe     path to npp.exe used by the open/run verbs (default: this repo's build)
# -Icon       path to the .ico (default: repo assets\npp-file.ico, or the installed copy)
# -Unregister removes the HKCU .npp association created by this script.

[CmdletBinding()]
param(
    [string]$NppExe,
    [string]$Icon,
    [switch]$Unregister
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

if (-not $NppExe) {
    $candidate = Join-Path $root 'build\npp.exe'
    if (Test-Path $candidate) { $NppExe = $candidate }
    else {
        $cmd = Get-Command npp.exe -ErrorAction SilentlyContinue
        if ($cmd) { $NppExe = $cmd.Source }
    }
}
if (-not $Icon) {
    $repoIcon = Join-Path $root 'assets\npp-file.ico'
    $installed = Join-Path $env:LOCALAPPDATA 'Programs\N++\share\npp\npp-file.ico'
    if (Test-Path $repoIcon) { $Icon = $repoIcon }
    elseif (Test-Path $installed) { $Icon = $installed }
}

# RegistryKey.SetValue preserves embedded quotes (unlike reg.exe /d) and
# does not disturb sibling values the way New-Item -Force can.
function Reg-Default([string]$Key, [string]$Value) {
    $k = [Microsoft.Win32.Registry]::CurrentUser.CreateSubKey($Key)
    $k.SetValue('', $Value)
    $k.Close()
}
function Reg-Value([string]$Key, [string]$Name, [string]$Value) {
    $k = [Microsoft.Win32.Registry]::CurrentUser.CreateSubKey($Key)
    $k.SetValue($Name, $Value)
    $k.Close()
}

if ($Unregister) {
    Remove-Item -Recurse -Force 'HKCU:\Software\Classes\.npp' -ErrorAction SilentlyContinue
    Remove-Item -Recurse -Force 'HKCU:\Software\Classes\NPP.SourceFile' -ErrorAction SilentlyContinue
    ie4uinit.exe -show 2>$null | Out-Null
    Write-Host 'Removed .npp file association (HKCU).'
    return
}

if (-not $NppExe -or -not (Test-Path $NppExe)) {
    throw "npp.exe not found; pass -NppExe <path> (looked at build\npp.exe and PATH)."
}
if (-not $Icon -or -not (Test-Path $Icon)) {
    throw "npp-file.ico not found; pass -Icon <path>."
}

$iconRef = '"' + $Icon + '"'
$exe = '"' + $NppExe + '"'

Reg-Default 'Software\Classes\.npp' 'NPP.SourceFile'
Reg-Value  'Software\Classes\.npp' 'Content Type' 'text/plain'

Reg-Default 'Software\Classes\NPP.SourceFile' 'N++ source file'
Reg-Value  'Software\Classes\NPP.SourceFile' 'DefaultIcon' $iconRef
Reg-Default 'Software\Classes\NPP.SourceFile\shell\open\command' "$exe `"%1`""
Reg-Default 'Software\Classes\NPP.SourceFile\shell\run' 'Run with N++'
Reg-Value  'Software\Classes\NPP.SourceFile\shell\run' 'Icon' $iconRef
Reg-Default 'Software\Classes\NPP.SourceFile\shell\run\command' "$exe run `"%1`""

# Refresh Explorer so the new icon shows up without a reboot.
ie4uinit.exe -show 2>$null | Out-Null

Write-Host "Associated .npp with N++:"
Write-Host "  open : $exe `"%1`""
Write-Host "  run  : $exe run `"%1`""
Write-Host "  icon : $Icon"
