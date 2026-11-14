#!/usr/bin/env python3
"""Repository-wide formatter and lint gate.

The checkout root carries a ``npp.toml`` so the compiler treats the whole
repository as one project. This test proves that gate stays usable: every
shipped ``.npp`` source passes ``npp fmt --check`` and ``npp lint``, and the
root manifest itself parses.

``selfhost/tests/*.npp`` are program fragments consumed by the self-host suite,
not standalone programs, so they are exempt from the lint pass. The
negative-test directory also contains deliberately ill-typed programs that
must be rejected by the compiler's semantic tests, not linted as applications.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

SKIP_DIRS = {
    ".git", "build", "dist", "site", "vendor", "node_modules", "CMakeFiles",
    "target", ".cache", ".venv",
}
SKIP_LINT = (
    Path("selfhost") / "tests",
    Path("tests/fuzz/corpus"),
    Path("tests/negative"),
)
# Fuzz corpus files are parser/lexer test inputs, not shipped source programs.
# Several intentionally contain incomplete or invalid syntax.
SKIP_FORMAT = (Path("tests/fuzz/corpus"),)


def sources(root: Path) -> list[Path]:
    found: list[Path] = []
    for path in sorted(root.rglob("*.npp")):
        relative = path.relative_to(root)
        if any(
            part in SKIP_DIRS or part.startswith("build-")
            for part in relative.parts[:-1]
        ):
            continue
        found.append(path)
    return found


def run(npp: Path, *arguments: str, cwd: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(npp), *arguments],
        cwd=cwd,
        capture_output=True,
        text=True,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--npp", type=Path, required=True, help="path to the built compiler")
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    args = parser.parse_args()
    root = args.root.resolve()

    manifest = root / "npp.toml"
    if not manifest.is_file():
        raise SystemExit(f"repository manifest is missing: {manifest}")

    info = run(args.npp, "info", cwd=root)
    if info.returncode != 0 or "npp-repository" not in info.stdout:
        raise SystemExit(f"npp info rejected the repository manifest:\n{info.stdout}{info.stderr}")

    all_sources = sources(root)
    formatted_count = 0
    for path in all_sources:
        relative = path.relative_to(root)
        if any(relative.parts[: len(prefix.parts)] == prefix.parts for prefix in SKIP_FORMAT):
            continue
        formatted = run(args.npp, "fmt", "--check", str(path), cwd=root)
        if formatted.returncode != 0:
            raise SystemExit(
                f"npp fmt --check failed for {relative}; run `npp fmt {relative}`:\n"
                f"{formatted.stdout}{formatted.stderr}"
            )
        formatted_count += 1

    linted = 0
    warnings = 0
    for path in all_sources:
        relative = path.relative_to(root)
        if any(relative.parts[: len(prefix.parts)] == prefix.parts for prefix in SKIP_LINT):
            continue
        result = run(args.npp, "lint", str(path), cwd=root)
        if result.returncode != 0:
            raise SystemExit(f"npp lint failed for {relative}:\n{result.stdout}{result.stderr}")
        if "NX9001" in result.stdout:
            raise SystemExit(f"{relative}: trailing whitespace is not formatter output")
        warnings += result.stdout.count("NX9002")
        linted += 1

    print(
        f"PASS repository sources: {len(all_sources)} .npp files, {linted} linted, "
        f"{warnings} TODO/FIXME marker(s), {formatted_count} formatted, formatter clean"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
