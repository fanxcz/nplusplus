# N++

![N++ mark](docs/assets/images/npp-n-2026.svg)

**N++ is an open-source, statically checked language for native applications, developer tools, and game prototypes.** Its compiler is written in C++20 and combines a built-in x86-64 backend for a documented subset with an LLVM-based path for broader language support. The current tree includes fixed-width numeric types, checked read-only/mutable slices, an SDL-based 2D helper layer, and an experimental compatibility-OpenGL 3D path.

[![CI](https://github.com/fanxcz/nplusplus/actions/workflows/build.yml/badge.svg)](https://github.com/fanxcz/nplusplus/actions/workflows/build.yml)
[![Quality gates](https://github.com/fanxcz/nplusplus/actions/workflows/quality.yml/badge.svg)](https://github.com/fanxcz/nplusplus/actions/workflows/quality.yml)
[![Version](https://img.shields.io/badge/version-1.0.0-informational.svg)](CHANGELOG.md)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/fanxcz/nplusplus/blob/main/LICENSE)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)

**Repository:** https://github.com/fanxcz/nplusplus  
**Language name:** N++ — sources use the `.npp` extension and the `npp` command-line tool.

## What N++ is

N++ aims to make it practical to build native software with one language: small desktop tools, command-line utilities, creative applications, and game prototypes. The repository contains a minimal native-window app template, a CLI template, the N++ Paint application example, project tooling, a language server, and an interactive REPL.

Highlights:

- **Two backends.** `--backend native` emits machine code directly for a documented x86-64 subset with no external compiler; `--backend llvm` lowers the full language to LLVM IR and links through a configured C++ toolchain. `--backend auto` (default) picks automatically.
- **Statically checked.** Types, ownership-style mutability rules, exhaustiveness-checked `match`, and compile-time diagnostics are enforced before code generation.
- **Batteries included.** `stdlib/` provides modules for I/O, files, JSON, networking, SQLite, crypto, math, collections, UI, sprites, tilemaps, physics, particles, and audio.
- **Self-hosting.** `selfhost/` contains lexer, parser, type-checker and codegen stages written in N++ itself.
- **Tooling.** `npp new|build|run|check|fmt|lint|doc|bench|test|pkg|lsp|doctor|repl`, plus a VS Code extension in `editors/vscode/` and the N++ Studio desktop workspace in `apps/studio/`.

## Quick start

```bash
git clone https://github.com/fanxcz/nplusplus.git
cd nplusplus
cmake --preset=release
cmake --build --preset=release

./build/npp --version
./build/npp self-test
```

See [Installation](docs/getting-started/installation.md) for platform prerequisites and [the quickstart](docs/getting-started/quickstart.md) for your first program.

```bash
npp new HelloNpp --template cli
cd HelloNpp
npp run
```

## Repository layout

| Path | Contents |
|---|---|
| `src/` | Compiler (C++20): lexer, parser, semantic analysis, LLVM and native backends, driver, LSP, REPL |
| `src/runtime/` | Runtime linked into generated applications |
| `stdlib/` | Standard library modules (`import std.io`, `std.fs`, …) |
| `selfhost/` | Self-hosted compiler stages in N++ |
| `examples/` | Runnable examples plus `templates/` used by `npp new` |
| `tests/` | Unit tests, smoke tests, fixtures, fuzz harnesses |
| `docs/` | Language reference, runtime docs, architecture, contributor guides |
| `apps/studio/` | N++ Studio desktop application |
| `editors/vscode/` | VS Code extension |
| `scripts/` | Repository maintenance scripts |

See [Repository architecture](docs/architecture/repository-layout.md) for details.

## Documentation

- [Language reference](docs/language-spec.md) and [manual](docs/manual.md)
- [Language guide](docs/language/) — syntax, types, functions, control flow, modules, error handling, FFI
- [Standard library](docs/std/) — per-module reference
- [Runtime](docs/runtime/) — graphics, audio, input, UI, ECS, physics
- [Tooling](docs/tooling/) — CLI commands, formatter, linter, LSP, benchmarks
- [Building from source](docs/getting-started/installation.md) · [Contributing](CONTRIBUTING.md)

## Examples

`examples/` contains runnable programs covering the language and runtime: `hello.npp`, `control_flow.npp`, `structs.npp`, `enums_match.npp`, `memory.npp`, `ffi.npp`, `ui_playground.npp`, `npp_paint.npp`, `neon_survivor.npp`, and more. `examples/templates/` holds the `npp new` project templates (`cli`, `app`, `game`, `3d`, `paint`, `server`, `library`).

## Development

```bash
cmake --preset=release
cmake --build --preset=release
ctest --test-dir build --output-on-failure
```

CI runs the full matrix on Ubuntu, Windows and macOS (`.github/workflows/build.yml`, `quality.yml`). See [contributing](docs/development/contributing.md) and [testing](docs/development/testing.md).

## License

[MIT](LICENSE)
