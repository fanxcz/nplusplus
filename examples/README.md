# Examples

This directory contains small programs used to demonstrate N++ language features, runtime APIs, games, and desktop tools. The examples are kept as source files so they can be checked and run with the compiler.

For the broader feature guides, start at [the examples documentation](../docs/examples/index.md).

## Start here

- [`hello.npp`](hello.npp) — smallest runnable program.
- [`ui_playground.npp`](ui_playground.npp) — interactive native-app UI widgets and layout helpers.
- [`game2d_starter.npp`](game2d_starter.npp) — self-contained 2D starter using the standard game helper modules.
- [`new_features_demo.npp`](new_features_demo.npp) — compact feature showcase.
- [`native_graphics_game.npp`](native_graphics_game.npp) — simple native 2D game loop.
- [`game2d_platformer.npp`](game2d_platformer.npp) — tilemap-based platformer.

## Language basics

Small, focused source files cover arrays, loops, branching, functions, structs, enums, pointers, memory, floats, imports, and FFI. Representative files include `arrays.npp`, `break_continue.npp`, `control_flow.npp`, `else_if.npp`, `enums_match.npp`, `factorial.npp`, `ffi.npp`, `float.npp`, `imports.npp`, `lists_demo.npp`, `memory.npp`, `pointers.npp`, `structs.npp`, and `utility.npp`.

## Games and graphical runtime

- [`game2d_starter.npp`](game2d_starter.npp) — an asset-free platformer sandbox demonstrating tilemap, collision, physics, camera and animation helpers.

- [`neon_survivor.npp`](neon_survivor.npp) — arena survival game.
- [`game2d_platformer.npp`](game2d_platformer.npp) — 2D tilemap platformer.
- [`npp_arena.npp`](npp_arena.npp) and [`terminal_game.npp`](terminal_game.npp) — terminal game examples.
- [`audio_demo.npp`](audio_demo.npp), [`fonts_demo.npp`](fonts_demo.npp), [`model_demo.npp`](model_demo.npp), [`native_3d.npp`](native_3d.npp), and [`particles_demo.npp`](particles_demo.npp) — focused runtime demonstrations.
- `v080_game_framework.npp` and `v080_runtime_showcase.npp` — explicitly version-labelled legacy examples.

## Interactive applications

- [`ui_playground.npp`](ui_playground.npp) — settings panel with buttons, checkboxes, slider, progress bar and layout cells.

`npp_paint.npp`, `npp_editor.npp`, `npp_focus.npp`, `npp_studio.npp`, `task_manager.npp`, and `interactive_cli.npp` demonstrate larger interactive applications and workflows.

## Templates

Starter files live in [`templates/app.npp`](templates/app.npp), [`templates/game.npp`](templates/game.npp), and the other files in `templates/`. Some are intentionally similar to complete demos: keep these copies when updating project scaffolding, and do not treat them as accidental duplicate files.

## Check and run

From the repository root:

```sh
npp check examples/hello.npp
npp run examples/hello.npp
npp check examples/vector_lab.npp
npp run examples/vector_lab.npp
```

Some examples load assets through repository-relative paths. Run them from the repository root unless their own launcher changes the working directory. See [`assets/README.md`](assets/README.md) before moving or removing files under `assets/`.
