# Repository Architecture

N++ (tooling prefix `npp`) is organized as a layered native toolchain. The source tree is grouped by responsibility.

## Repository root

```text
nplusplus/
├── .github/
│   ├── workflows/        # CI: build, quality gates, Studio packaging
│   └── ISSUE_TEMPLATE/
├── apps/
│   └── studio/           # N++ Studio desktop application (Electron)
├── docs/                 # User, architecture and contributor documentation
├── editors/
│   └── vscode/           # VS Code language extension
├── examples/             # Runnable examples, assets and project templates
├── scripts/              # Repository maintenance scripts
├── selfhost/             # Self-hosted compiler stages written in N++
├── src/                  # Compiler and runtime sources (C++20)
├── stdlib/               # Importable standard-library modules (*.npp)
└── tests/                # C++ unit tests, compiler smoke tests and fixtures
```

Root files such as `CMakeLists.txt`, `CMakePresets.json`, `README.md`,
`CHANGELOG.md`, `LICENSE`, `npp.toml` and `.clang-format` remain at the root.
Starter sources used by `npp new` live in `examples/templates/`; built packages
retain the user-facing `project-templates/` directory name for compatibility.

## Source layout

| Path | Responsibility |
|---|---|
| `src/cli/` | Process entry point and CLI bootstrap |
| `src/compiler/ast/` | Shared syntax-tree model |
| `src/compiler/frontend/lexer/` | Source text to position-aware tokens |
| `src/compiler/frontend/parser/` | Tokens to AST |
| `src/compiler/semantic/` | Name and type validation |
| `src/compiler/backend/codegen/` | AST to LLVM IR |
| `src/compiler/backend/native/` | Built-in x86-64 machine-code backend |
| `src/compiler/driver/` | CLI commands, project workflows, LSP and toolchain orchestration |
| `src/runtime/` | Native runtime linked into generated applications |
| `stdlib/` | Modules imported as `std.io`, `std.fs`, and similar |
| `selfhost/` | Lexer/parser/type-checker/codegen stages written in N++ itself |
| `examples/templates/` | Starter sources used by `npp new` |
| `examples/` | Runnable language and runtime examples |
| `tests/` | Unit and CLI regression tests |
| `docs/` | User and contributor guides |
| `apps/studio/` | N++ Studio desktop application |
| `editors/vscode/` | VS Code extension: grammar, snippets, language configuration |

## Dependency direction

```text
CLI → Driver → Frontend (Lexer → Parser → AST)
              → Semantic Analysis
              → Backend (LLVM IR or built-in native emitter)
              → Native Toolchain + Runtime/stdlib resolution
```

The AST is the shared boundary between parsing, semantic analysis and code generation. The runtime and standard library are resolved through explicit source-tree, environment, installed and portable-package paths.

## Change rules

- Keep the public `.npp` language syntax and CLI stable unless a language change is explicitly versioned.
- Keep the front end independent from platform-specific process launching.
- Give diagnostics stable error codes and preserve source file/line/column when the lexer or parser knows them.
- Add an executable regression test for every bug fix that can be expressed as a small program.
- Run CMake build, CTest, and Windows/Linux/macOS CI before merging architectural changes.
