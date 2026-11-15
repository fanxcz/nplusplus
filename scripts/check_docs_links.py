#!/usr/bin/env python3
"""Verify that relative Markdown links in docs/ and top-level *.md resolve.

Checks ``[text](target.md)`` style links only; external URLs, anchors and
non-Markdown assets are skipped. Exits non-zero and prints every broken link.
"""
from __future__ import annotations

import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LINK_RE = re.compile(r"\]\(([^)\s]+)\)")
SKIP_DIRS = {".git", "build", "node_modules", "dist", "release"}


def markdown_files() -> list[Path]:
    files = [p for p in ROOT.glob("*.md")]
    for base in (ROOT / "docs", ROOT / "apps", ROOT / "editors"):
        if base.is_dir():
            files.extend(base.rglob("*.md"))
    return sorted(p for p in files if not SKIP_DIRS & set(p.parts))


def main() -> int:
    broken: list[tuple[Path, str]] = []
    for path in markdown_files():
        try:
            text = path.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError):
            continue
        for match in LINK_RE.finditer(text):
            link = match.group(1).split("#", 1)[0]
            if not link or link.startswith(("http://", "https://", "mailto:", "/")):
                continue
            target = (path.parent / link).resolve()
            if not target.exists():
                broken.append((path.relative_to(ROOT), link))
    if broken:
        for src, link in broken:
            print(f"BROKEN {src}: {link}", file=sys.stderr)
        print(f"{len(broken)} broken link(s)", file=sys.stderr)
        return 1
    print("docs link check: OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
