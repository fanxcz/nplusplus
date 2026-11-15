# Examples Overview

The `examples/` directory contains programs for compiler/language checks, simple applications, games, graphics, audio and runtime helpers. Examples are standalone `.npp` files unless stated otherwise.

## Run an example

From the repository root:

```bash
npp check examples/hello.npp
npp run examples/hello.npp
```

The CLI accepts a source-file path directly; `npp run --example NAME` is not a supported command. If a source relies on relative assets, keep the repository root as the working directory when running it.

## Language basics

| Source | Demonstrates |
|---|---|
| `hello.npp` | Basic function and output |
| `control_flow.npp`, `else_if.npp`, `break_continue.npp` | Conditional branches and loops |
| `arrays.npp`, `structs.npp`, `enums_match.npp` | Data types and pattern matching |
| `imports.npp` | Project-relative module imports |
| `factorial.npp` | Recursion |
| `float.npp` | Floating-point values |
| `memory.npp`, `pointers.npp` | Low-level memory/pointer examples |
| `interactive_cli.npp` | Prompted terminal input |

## Applications and games

| Source | What it actually contains |
|---|---|
| `npp_focus.npp` | A graphical focus timer using mouse controls, sound tone and a small text session file |
| `npp_paint.npp` | Raster drawing canvas with brush/eraser/line/rectangle/fill tools, undo/redo and BMP save/load |
| `npp_editor.npp` | Experimental 2D scene/box editor built from drawing, UI helper, ECS and scene snapshot APIs |
| `neon_survivor.npp` | Arena survivor prototype with player movement, mouse shooting and enemy waves/state |
| `npp_arena.npp` | Terminal turn-based hero-versus-boss combat simulation |
| `terminal_game.npp` | Minimal terminal-grid movement game |
| `npp_studio.npp` | Terminal menu demo containing calculator, guessing and mini-battle routines |

The file `examples/npp_studio.npp` is a terminal menu demonstration. The names describe individual sample programs, not claims that each is a complete production product or game framework.

## Graphics and runtime

| Source | Demonstrates |
|---|---|
| `native_app.npp` | Window, text, rectangle and mouse polling |
| `native_graphics_game.npp` | Simple 2D movement, mouse state and primitive drawing |
| `game2d_platformer.npp` | Tilemap platforms, axis-separated collision resolution, gravity and jumping |
| `native_3d.npp` | 3D grid/cubes and camera setup |
| `model_demo.npp` | Load and draw the repository's OBJ cube asset |
| `particles_demo.npp` | Create, update, draw and clear simple runtime particles |
| `audio_demo.npp` | Load and play a WAV file through the optional runtime audio backend |
| `ultimate_showcase.npp` | Small integrated runtime showcase; inspect source for exact scope |

## Source templates

Templates live in `examples/templates/`. The `npp new` command copies one selected template to a generated project's `src/main.npp`; it does not create a multi-module framework. See [Project Templates](../templates/index.md).
