# Changelog

All notable changes to N++ are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - Unreleased

### Added

- `std.sys` module: `sys_exit`, `sys_beep`, `sys_key_pending`,
  `sys_read_key`, `sys_read_key_char`.
- `std.string`: `char_count`, `char_at`, `from_char`, `valid_utf8` wrappers.
- `std.text`: `text_char_count`, `text_valid_utf8`, `text_lines`,
  `text_remove_prefix`, `text_remove_suffix`.
- `std.collections`: `int_list_shuffle`/`int_list_pick` and
  `string_list_shuffle`/`string_list_pick` (Fisher-Yates + uniform pick).
- Bug fix: `text_ends_with` and `text_remove_prefix` used `string.slice`
  with an end index where a length was intended.
- Documentation: `docs/cheatsheet.md`, `docs/guides/` (graphics, audio,
  game loop, UI), `docs/std/sys.md`.
- Examples: `sys_demo.npp`, `collections_demo.npp`.
- VS Code extension: Check/Build/Run/Format commands with keybindings and
  a configurable `npp.compilerPath`.
- Release pipeline: NSIS `npp-setup` Windows installer, N++ Studio
  installer artifacts, SHA-256 sidecars for every asset.
- `scripts/uninstall.sh` / `uninstall.ps1`; `check_versions.py` now also
  enforces a docs page per stdlib module.
- `.clang-format`: `SortIncludes` disabled — runtime `.inc` files depend
  on deliberate include order.

## [1.0.0] - 2026-11-14

First release of N++, the renamed continuation of the NEXUS language project.

### Added

- C++20 compiler with a built-in x86-64 machine-code backend for a documented
  language subset and an LLVM backend for the full language.
- Statically checked language: fixed-width numeric types, checked slices,
  exhaustive `match`, enums with payloads, `defer`, and `?` error propagation.
- Standard library modules under `stdlib/` for I/O, files, JSON, networking,
  SQLite, crypto, math, collections, UI, sprites, tilemaps, physics, particles
  and audio.
- Project tooling: `npp new`, `build`, `run`, `check`, `fmt`, `lint`, `doc`,
  `bench`, `test`, `pkg`, `lsp`, `doctor`, `repl` and `self-test`.
- Language server (`npp lsp`), interactive REPL, and a VS Code extension in
  `editors/vscode/`.
- N++ Studio desktop workspace in `apps/studio/`.
- Self-hosted compiler stages (lexer, parser, type checker, codegen) in
  `selfhost/`.
- Comprehensive test suite: unit tests, per-feature smoke tests, negative
  diagnostics, backend parity and differential checks, plus opt-in libFuzzer
  harnesses.

### Changed

- Language and tooling renamed from NEXUS to N++: sources use the `.npp`
  extension, the CLI is `npp`, project manifests use `npp.toml`, and the
  repository lives at `github.com/fanxcz/nplusplus`.
