# Remove an N++ installation created by install.ps1 or the NSIS installer.
#
#   irm https://raw.githubusercontent.com/fanxcz/nplusplus/main/scripts/uninstall.ps1 | iex
#
# Environment overrides:
#   NPP_PREFIX    install root to remove (default: %LOCALAPPDATA%\NPP)
$ErrorActionPreference = 'Stop'

$Prefix = if ($env:NPP_PREFIX) { $env:NPP_PREFIX } else { Join-Path $env:LOCALAPPDATA 'NPP' }

if (-not (Test-Path $Prefix)) {
    Write-Host "nothing to remove: $Prefix does not exist"
    exit 0
}

Write-Host "removing N++ from $Prefix"
Remove-Item -LiteralPath $Prefix -Recurse -Force
Write-Host "done. Remove $Prefix\bin from PATH if you added it manually."
