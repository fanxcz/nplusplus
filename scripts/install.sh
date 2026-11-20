#!/usr/bin/env bash
# Install N++ from the latest GitHub release.
#
#   curl -fsSL https://raw.githubusercontent.com/fanxcz/nplusplus/main/scripts/install.sh | bash
#
# Environment overrides:
#   NPP_VERSION   install a specific tag (default: latest release)
#   NPP_PREFIX    install root (default: ~/.local/npp)
set -euo pipefail

REPO="fanxcz/nplusplus"
PREFIX="${NPP_PREFIX:-$HOME/.local/npp}"

info() { printf '\033[1;34m==>\033[0m %s\n' "$*"; }
die()  { printf '\033[1;31merror:\033[0m %s\n' "$*" >&2; exit 1; }

command -v curl >/dev/null 2>&1 || die "curl is required"
command -v tar   >/dev/null 2>&1 || die "tar is required"
command -v python3 >/dev/null 2>&1 || command -v python >/dev/null 2>&1 || die "python3 is required for JSON parsing"
PY="$(command -v python3 || command -v python)"

os="$(uname -s)"
arch="$(uname -m)"
case "$os" in
  Linux)  os=linux ;;
  Darwin) os=macos ;;
  *)      die "unsupported OS: $os (use install.ps1 on Windows)" ;;
esac
case "$arch" in
  x86_64|amd64) arch=x86_64 ;;
  arm64|aarch64) arch=arm64 ;;
  *) die "unsupported architecture: $arch" ;;
esac
asset="npp-${os}-${arch}.tar.gz"
# Only macOS arm64 binaries are published; Linux is x86_64-only for now.
if [ "$os" = macos ] && [ "$arch" = x86_64 ]; then
  die "no macOS x86_64 build is published; use Rosetta with the arm64 build or build from source"
fi
if [ "$os" = linux ] && [ "$arch" = arm64 ]; then
  info "note: linux/arm64 builds may not be published yet"
fi

if [ -n "${NPP_VERSION:-}" ]; then
  tag="$NPP_VERSION"
  base="https://github.com/${REPO}/releases/download/${tag}"
else
  info "resolving latest release of ${REPO}"
  latest="$(curl -fsSL "https://api.github.com/repos/${REPO}/releases/latest")"
  tag="$(printf '%s' "$latest" | "$PY" -c 'import json,sys; print(json.load(sys.stdin)["tag_name"])')" || die "could not parse latest release"
  base="https://github.com/${REPO}/releases/download/${tag}"
fi
info "installing ${tag} (${os}/${arch}) into ${PREFIX}"

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

curl -fsSL "${base}/${asset}" -o "${tmp}/npp.tar.gz"

# Verification is mandatory: a missing sidecar is a hard failure, never a
# reason to execute an unverified archive.
curl -fsSL "${base}/${asset}.sha256" -o "${tmp}/npp.tar.gz.sha256" \
  || die "checksum sidecar for ${asset} is missing; refusing to install"
if command -v sha256sum >/dev/null 2>&1; then
  (cd "$tmp" && sha256sum -c npp.tar.gz.sha256) || die "checksum mismatch"
else
  (cd "$tmp" && shasum -a 256 -c npp.tar.gz.sha256) || die "checksum mismatch"
fi

mkdir -p "$PREFIX"
tar -xzf "${tmp}/npp.tar.gz" -C "$PREFIX"

bin="$PREFIX/bin/npp"
[ -x "$bin" ] || die "installed binary not found at ${bin}"
info "installed: $("$bin" --version 2>/dev/null | head -1 || echo "npp")"
cat <<EOF

Add the N++ tools to your PATH:

  export PATH="${PREFIX}/bin:\$PATH"

Verify with:  npp --version && npp doctor
EOF
