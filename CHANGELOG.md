# Changelog

All notable changes to N++ are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.2.2] - Unreleased

### Added

- Native backend aggregate support: fixed-size arrays (`[T; n]`
  annotations, `[a, b, c]` literals, bounds-checked `xs[i]` reads and
  `xs[i] = v` writes, `len(array)`) and structs (fields, `P { ... }`
  literals, `p.field` access/assignment, nesting). Aggregates are
  arena-allocated and deep-copied on `let`/`=`/call boundaries, matching
  value semantics; they can be passed to and returned from functions.
  Compound assignment (`+=`, `-=`, `*=`, `/=`, `%=`) works on variables,
  struct fields and array elements (including `string +=`), and
  `for x in array` iterates element copies.
- New native smoke fixtures: `arrays.npp`, `structs.npp`.
- Native backend `f64` support: SSE2 scalar arithmetic, `f64`
  parameters/returns, `print(f64)`/`str_f64` (shortest-round-trip decimal,
  fixed notation for exponents in `[-4, 14]`, scientific otherwise),
  `input_f64`/`str_to_f64` parsing, `to_f64`/`to_i64` conversions.
- New native smoke fixtures: `f64.npp`, `input_f64.npp`.
- Native backend pointers (`*T`, `&x`, `*p` read/write) for output
  parameters and raw memory, plus `mem_alloc`/`mem_read`/`mem_set`/
  `mem_copy`/`mem_free`.
- Native backend system surface: `os.args_count`/`os.args_get`, `env`,
  `file_exists`, `file_read`/`file_read_checked`/`file_write`,
  `beep`, `key_pressed`, `read_key`.
- Native backend string builtins: `string.len`, `string.byte_at`,
  `string.char_at`, `string.chars`, `string.contains`, `string.find`,
  `string.slice`, `string.split_at`, `string.split_count`,
  `string.trim`, `string.upper`, `string.lower`, `string.from_char`,
  `string.valid_utf8` (UTF-8 aware), used by `std.string`/`std.text`.
- Native backend collections: `ilist_*` and `slist_*` handles (new,
  push, push_checked, len, get/try_get/set/set_checked/try_pop, insert,
  remove, reserve, clear, free) with `*T` out-parameters, and top-level
  `defer call()` support used by `std.collections` cleanup.
- Native backend math: `sin`, `cos`, `tan`, `sqrt`, `floor`, `ceil`,
  `atan2`, `pow` (x87 FPU), and physics helpers `physics_aabb` /
  `physics_circle` (SSE2).
- Native-compilable stdlib modules: `io`, `os`, `env`, `fs`, `sys`,
  `string`, `text`, `math`, `random`, `time`, `memory`, `collections`,
  `physics2d`, `collision2d`, `tilemap`, `vec2`, `vec3`, `animation2d`.
- New native smoke fixtures: `lists.npp`, `math_phys.npp`, `defer.npp`.

### Fixed

- `npp run` / direct `.npp` invocation now also pauses on interactive
  consoles when compilation fails, so error messages stay visible.

## [1.2.1] - 2026-11-15

### Added

- `npp run` and direct `.npp` file invocation now pause with
  `Press Enter to exit...` when stdin is an interactive console, so the
  window stays open after the program finishes. Piped input is unaffected.

### Fixed

- LLVM backend: added missing `npp_json_valid`, `npp_json_type`,
  `npp_json_array_len` and `npp_json_array_get` declarations, fixing
  compilation of programs using `std.json` wrappers such as
  `examples/npp_v1_full_stack.npp`.

## [1.2.0] - 2026-11-15

### Added

- Native backend builtins: `exit`, `time_ms`, `sleep`, `random_i64`,
  `str_bool`, `input_i64`, `file_exists`, `os.args_count`,
  `os.args_get`, `string.len`, `string.byte_at`, plus reworked Windows
  `ReadFile`/`WriteFile` paths and Linux syscall I/O.
- New native smoke fixtures: `sys_io.npp`, `exit_builtin.npp`,
  `input_i64.npp`.

### Removed

- N++ Studio (`apps/studio`) and its CI workflow. Releases now contain
  only the compiler toolchain.

### Changed

- NSIS installer registers `.npp` files under `HKCU\Software\Classes`
  with `open` and `run` shell verbs (`npp <file>` runs the program) and
  removes the PATH entry plus associations on uninstall.

## [1.1.0] - 2026-11-15

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
- Release pipeline: NSIS `npp-setup` Windows installer (registers `.npp`
  files), SHA-256 sidecars for every asset.
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
- Self-hosted compiler stages (lexer, parser, type checker, codegen) in
  `selfhost/`.
- Comprehensive test suite: unit tests, per-feature smoke tests, negative
  diagnostics, backend parity and differential checks, plus opt-in libFuzzer
  harnesses.

### Changed

- Language and tooling renamed from NEXUS to N++: sources use the `.npp`
  extension, the CLI is `npp`, project manifests use `npp.toml`, and the
  repository lives at `github.com/fanxcz/nplusplus`.
