#!/usr/bin/env bash
# Register the .npp file icon + "open/run with N++" on Linux (no root needed).
#
#   scripts/set-file-icon.sh [path-to-npp-binary]
#
# Installs a hicolor icon, a text/x-npp MIME type, and a desktop entry into
# the per-user XDG dirs, then runs update-desktop-database / xdg-mime.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
npp_bin="${1:-$(command -v npp || true)}"
[ -n "$npp_bin" ] || { echo "npp binary not found; pass its path as \$1" >&2; exit 1; }

icon_src="$root/assets/npp-icon-256.png"
[ -f "$icon_src" ] || icon_src="$root/docs/assets/images/npp-logo-64.png"

xdg_data="${XDG_DATA_HOME:-$HOME/.local/share}"
apps="$xdg_data/applications"
icons="$xdg_data/icons/hicolor/256x256/apps"
mime_pkg="$xdg_data/mime/packages"
mkdir -p "$apps" "$icons" "$mime_pkg"

cp "$icon_src" "$icons/npp-file.png"

cat > "$mime_pkg/npp.xml" <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
  <mime-type type="text/x-npp">
    <comment>N++ source file</comment>
    <glob pattern="*.npp"/>
  </mime-type>
</mime-info>
EOF
command -v update-mime-database >/dev/null && update-mime-database "$xdg_data/mime" || true

cat > "$apps/npp.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=N++
Comment=N++ compiler / runner
Exec=$npp_bin %f
Icon=npp-file
MimeType=text/x-npp;
Categories=Development;
Terminal=true
EOF
chmod +x "$apps/npp.desktop"

command -v update-desktop-database >/dev/null && update-desktop-database "$apps" || true
xdg-mime default npp.desktop text/x-npp 2>/dev/null || true

echo "Registered .npp -> $npp_bin (icon: npp-file)"
