#!/usr/bin/env python3
"""Verify that compiler and VS Code extension versions agree.

Reads ``project(npp VERSION x.y.z)`` from CMakeLists.txt and the ``version``
field of editors/vscode/package.json, then fails when they disagree.
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CMAKE_PROJECT_RE = re.compile(r"project\(npp\s+VERSION\s+(\d+\.\d+\.\d+)", re.IGNORECASE)
SEMVER_RE = re.compile(r"^\d+\.\d+\.\d+$")


def read_version(path: Path, kind: str) -> str:
    try:
        text = path.read_text(encoding="utf-8")
    except OSError as exc:
        raise SystemExit(f"cannot read {path}: {exc}")
    if kind == "cmake":
        match = CMAKE_PROJECT_RE.search(text)
        if not match:
            raise SystemExit(f"no project(npp VERSION x.y.z) found in {path}")
        return match.group(1)
    try:
        data = json.loads(text)
    except json.JSONDecodeError as exc:
        raise SystemExit(f"invalid JSON in {path}: {exc}")
    version = data.get("version")
    if not isinstance(version, str):
        raise SystemExit(f"missing version in {path}")
    return version


def main() -> int:
    sources = {
        "compiler (CMakeLists.txt)": read_version(ROOT / "CMakeLists.txt", "cmake"),
        "VS Code extension": read_version(ROOT / "editors/vscode/package.json", "json"),
    }
    for name, version in sources.items():
        if not SEMVER_RE.match(version):
            print(f"FAIL {name}: '{version}' is not x.y.z", file=sys.stderr)
            return 1
    unique = set(sources.values())
    if len(unique) != 1:
        for name, version in sources.items():
            print(f"  {name}: {version}", file=sys.stderr)
        print("FAIL versions disagree", file=sys.stderr)
        return 1
    print(f"versions OK: {sources['compiler (CMakeLists.txt)']}")

    # Every stdlib module must have a matching docs/std page.
    stdlib = ROOT / "stdlib"
    docs_std = ROOT / "docs" / "std"
    missing = [
        f"stdlib/{p.name}"
        for p in sorted(stdlib.glob("*.npp"))
        if not (docs_std / f"{p.stem}.md").is_file()
    ]
    if missing:
        for item in missing:
            print(f"FAIL missing docs page for {item}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
