# N++ Documentation

- [Language reference](language-spec.md) — formal syntax and semantics
- [Manual](manual.md) — guided tour of the language and toolchain
- [Migration from Rust](migration-from-rust.md) — conceptual mapping for Rust users
- [Cheat sheet](cheatsheet.md) — CLI, builtins and stdlib at a glance
- [Guides](guides/index.md) — task-oriented walkthroughs (graphics, audio, game loop, UI)

## Getting started

| Page | Contents |
|---|---|
| [Index](getting-started/index.md) | Prerequisites and overview |
| [Installation](getting-started/installation.md) | Release installers and source builds |
| [Quick start](getting-started/quickstart.md) | First program in minutes |
| [Project structure](getting-started/project-structure.md) | `npp.toml`, `src/`, layouts |

## Language

| Page | Contents |
|---|---|
| [Overview](language/overview.md) | Language goals and workflow |
| [Syntax](language/syntax.md) | Lexical structure and grammar |
| [Types](language/types.md) | Integers, floats, strings, bool, slices |
| [Functions](language/functions.md) | Declarations, calls, returns |
| [Control flow](language/control-flow.md) | `if`, `while`, `for`, `match`, `break`, `continue` |
| [Modules](language/modules.md) | `import std.*` and file imports |
| [Error handling](language/error-handling.md) | `Option`, `Result`, `?`, `defer` |
| [FFI](language/ffi.md) | Calling native code |

## Standard library

| Page | Contents |
|---|---|
| [Overview](std/overview.md) | Module map and limitations |
| [std.io](std/io.md) · [std.fs](std/fs.md) · [std.os](std/os.md) · [std.env](std/env.md) · [std.sys](std/sys.md) | Console, files, arguments, environment, process/keys |
| [std.text](std/text.md) · [std.string](std/string.md) · [std.math](std/math.md) | Text and math helpers |
| [std.time](std/time.md) · [std.random](std/random.md) · [std.shell](std/shell.md) | Clock, randomness, processes |
| [std.json](std/json.md) · [std.db](std/db.md) · [std.net](std/net.md) · [std.crypto](std/crypto.md) | Data and networking |
| [std.collections](std/collections.md) · [std.memory](std/memory.md) · [std.option](std/option.md) · [std.result](std/result.md) | Data structures |
| [std.vec2](std/vec2.md) · [std.vec3](std/vec3.md) | Vector math |
| [2D/game modules](std/engine2d.md) | engine2d, tilemap, collision2d, physics2d, sprite2d, animation2d, camera2d, tilemap_render, ui, audio2d |

## Runtime

| Page | Contents |
|---|---|
| [Overview](runtime/overview.md) | Runtime architecture |
| [Graphics 2D](runtime/graphics-2d.md) · [Graphics 3D](runtime/graphics-3d.md) | Rendering paths |
| [Audio](runtime/audio.md) · [Input](runtime/input.md) · [UI](runtime/ui.md) | Media and interaction |
| [Memory](runtime/memory.md) · [Scenes](runtime/scenes.md) · [ECS](runtime/ecs.md) | Systems programming |
| [Physics](runtime/physics.md) · [Particles](runtime/particles.md) | Simulation |
| [Voxel roadmap](runtime/voxel-roadmap.md) | 3D engine status |

## Tooling

| Page | Contents |
|---|---|
| [Commands](tooling/commands.md) | Full `npp` CLI reference |
| [Formatter](tooling/fmt.md) · [Linter](tooling/lint.md) · [Doc generator](tooling/doc.md) | Code quality tools |
| [LSP](tooling/lsp.md) · [VS Code](tooling/vscode-extension.md) · [Editor](tooling/editor.md) | Editor integration |
| [Bench](tooling/bench.md) · [Native backend](tooling/native-backend.md) · [Backend explainer](tooling/backend-explainer.md) | Performance and backends |

## Examples and templates

| Page | Contents |
|---|---|
| [Examples index](examples/index.md) · [Overview](examples/overview.md) | Runnable programs |
| [Games](examples/games.md) · [Graphics](examples/graphics.md) · [Audio](examples/audio.md) | Topic guides |
| [Templates](templates/index.md) | `npp new` project templates |
| [Packages](packages/index.md) | `npp pkg` workflow |

## Architecture and development

| Page | Contents |
|---|---|
| [Architecture](architecture.md) · [Repository layout](architecture/repository-layout.md) | Codebase map |
| [Compiler pipeline](architecture/compiler-pipeline.md) · [Memory model](architecture/memory-model.md) · [Runtime design](architecture/runtime-design.md) | Internals |
| [Building](development/building.md) · [Testing](development/testing.md) · [Debugging](development/debugging.md) · [Platform support](development/platform-support.md) · [Roadmap](development/roadmap.md) · [Contributing](development/contributing.md) | Contributor guides |
| [CMake](build/cmake.md) · [Targets](build/targets.md) · [Cross-compilation](build/cross-compilation.md) | Build system |
| [Applications](applications.md) · [Studio](../apps/studio/README.md) | Desktop apps |
| [Style guide](style-guide-npp.md) · [Troubleshooting](troubleshooting.md) · [Versioning](versioning.md) · [Reproducible builds](reproducible-builds.md) · [Backend matrix](backend-support-matrix.md) | Reference |
