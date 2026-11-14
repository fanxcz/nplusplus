# Graphics Examples

These examples use the built-in `gfx_*` API. The current names are `gfx_poll`, `gfx_begin`, `gfx_end`, `gfx_clear`, `gfx_rect`, `gfx_circle`, `gfx_line` and `gfx_text`; older names such as `gfx_begin_frame`, `gfx_delta_time` and `gfx_draw_rect` are not part of the current runtime.

## Minimal native app

Source: `examples/native_app.npp`.

```bash
npp check examples/native_app.npp
npp run examples/native_app.npp
```

## 2D game demo

Source: `examples/native_graphics_game.npp`. It demonstrates WASD movement, a mouse-controlled target, frame-delta movement, simple score tracking, circles, a line and text.

```bash
npp check examples/native_graphics_game.npp
npp run examples/native_graphics_game.npp
```

## 3D cubes and grid

Source: `examples/native_3d.npp`. It uses `gfx3d_begin`, `gfx3d_grid`, `gfx3d_cube` and `gfx3d_end`. The current runtime does not expose a general shader/material system through this public API.

```bash
npp check examples/native_3d.npp
npp run examples/native_3d.npp
```

## Model loading

Source: `examples/model_demo.npp`. It loads `examples/assets/cube.obj` with `gfx3d_model_load`, then draws and unloads the model. This is an example of the current experimental model loader, not evidence of general glTF/PBR/animation support.

## Particles and canvas

- `examples/particles_demo.npp` demonstrates the current basic 2D particle create/update/draw APIs.
- `examples/npp_paint.npp` demonstrates a larger raster-canvas app with BMP save/load and undo/redo.
- `examples/npp_focus.npp` demonstrates a practical small native desktop utility.

The built-in runtime and platform packaging determine which optional graphics libraries must be present. Keep the repository root as the working directory for examples which load files under `examples/assets/`.
