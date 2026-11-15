# Install N++ from the latest GitHub release (Windows).
#
#   irm https://raw.githubusercontent.com/fanxcz/nplusplus/main/scripts/install.ps1 | iex
#
# Environment overrides:
#   $env:NPP_VERSION  install a specific tag (default: latest release)
#   $env:NPP_PREFIX   install root (default: ~\.local\npp -> %LOCALAPPDATA%\NPP)
$ErrorActionPreference = 'Stop'

$Repo   = 'fanxcz/nplusplus'
$Prefix = if ($env:NPP_PREFIX) { $env:NPP_PREFIX } else { Join-Path $env:LOCALAPPDATA 'NPP' }

function Info($msg) { Write-Host "==> $msg" -ForegroundColor Cyan }
function Die($msg)  { Write-Host "error: $msg" -ForegroundColor Red; exit 1 }

$arch = if ([Environment]::Is64BitOperatingSystem) { 'x86_64' } else { Die 'only x86_64 Windows builds are published' }
$asset = "npp-windows-$arch.zip"

if ($env:NPP_VERSION) {
    $tag  = $env:NPP_VERSION
    $base = "https://github.com/$Repo/releases/download/$tag"
} else {
    Info "resolving latest release of $Repo"
    $latest = Invoke-RestMethod "https://api.github.com/repos/$Repo/releases/latest"
    $tag    = $latest.tag_name
    $base   = "https://github.com/$Repo/releases/download/$tag"
}
Info "installing $tag (windows/$arch) into $Prefix"

$tmp = Join-Path ([IO.Path]::GetTempPath()) ("npp-install-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $tmp | Out-Null
try {
    $zip = Join-Path $tmp $asset
    Invoke-WebRequest -Uri "$base/$asset" -OutFile $zip -UseBasicParsing
    $sumUrl = "$base/$asset.sha256"
    try {
        $expected = (Invoke-WebRequest -Uri $sumUrl -UseBasicParsing).Content.Trim().Split(' ')[0]
        $actual   = (Get-FileHash -Algorithm SHA256 -Path $zip).Hash.ToLowerInvariant()
        if ($actual -ne $expected) { Die 'checksum mismatch' }
    } catch [System.Net.WebException] {
        # No checksum published; continue without verification.
    }

    New-Item -ItemType Directory -Path $Prefix -Force | Out-Null
    Expand-Archive -Path $zip -DestinationPath $Prefix -Force
} finally {
    Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
}

$bin = Join-Path $Prefix 'bin\npp.exe'
if (-not (Test-Path $bin)) { Die "installed binary not found at $bin" }

# Persist the bin directory on the user PATH (idempotent).
$userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
$binDir   = Split-Path -Parent $bin
if (($userPath -split ';') -notcontains $binDir) {
    [Environment]::SetEnvironmentVariable('Path', "$userPath;$binDir", 'User')
    Info "added $binDir to your user PATH (restart your terminal to pick it up)"
}

Info "installed: $((& $bin --version) -join ' ')"
Write-Host ''
Write-Host 'Verify with:  npp --version ; npp doctor'
