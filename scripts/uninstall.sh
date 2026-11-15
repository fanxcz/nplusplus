#!/usr/bin/env bash
# Remove an N++ installation created by install.sh or the NSIS installer.
#
#   curl -fsSL https://raw.githubusercontent.com/fanxcz/nplusplus/main/scripts/uninstall.sh | bash
#
# Environment overrides:
#   NPP_PREFIX    install root to remove (default: ~/.local/npp)
set -euo pipefail

PREFIX="${NPP_PREFIX:-$HOME/.local/npp}"

if [ ! -d "$PREFIX" ]; then
  printf 'nothing to remove: %s does not exist\n' "$PREFIX"
  exit 0
fi

printf 'removing N++ from %s\n' "$PREFIX"
rm -rf "$PREFIX"
printf 'done. Remove %s/bin from your PATH if you added it manually.\n' "$PREFIX"
