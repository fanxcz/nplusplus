# Guide: 2D graphics

N++ ships a SDL2-backed 2D renderer reachable through the `gfx_*`
builtins and the friendlier `std.engine2d` wrappers. Two paths exist:

- **Low level** — `gfx_init`, `gfx_begin`, `gfx_rect`, `gfx_text`,
  `gfx_end`, `gfx_poll`, `gfx_should_close` …
- **High level** — `std.engine2d` (`engine2d_init`,
  `engine2d_begin_frame`, `engine2d_draw_rect`, `engine2d_draw_text`,
  `engine2d_end_frame`, input helpers).

## Minimal window

```npp
import std.engine2d

fn main() {
    if !engine2d_init(640, 480, "Demo") { return }
    while !engine2d_should_close() {
        engine2d_begin_frame(0.08, 0.09, 0.12, 1.0)
        engine2d_draw_rect(100.0, 100.0, 200.0, 120.0, 0.2, 0.6, 0.9, 1.0)
        engine2d_draw_text(110.0, 110.0, 2.0, "Hello N++", 1.0, 1.0, 1.0, 1.0)
        engine2d_end_frame()
    }
    engine2d_shutdown()
}
```

`engine2d_begin_frame` clears the screen to the given RGBA colour;
`engine2d_end_frame` presents. Always pair them inside the loop.

## Sprites and textures

`std.sprite2d` wraps texture loading and drawing:

```npp
import std.sprite2d

let tex = sprite2d_load_texture("assets/player.png")
// returns a handle <= 0 on failure; check sprite2d_texture_width(tex)
sprite2d_draw_texture(tex, x, y, w, h)
sprite2d_unload_texture(tex)
```

`sprite2d_draw_frame` draws one cell of a sprite sheet; tinted variants
multiply the texture colour.

## Text

`engine2d_draw_text(x, y, scale, value, r, g, b, a)` uses the built-in
font. `std.engine2d` also exposes `gfx_font_load` for custom fonts —
see `examples/fonts_demo.npp`.

## Cameras and 3D

- `std.camera2d` — 2D camera helpers.
- Raw `gfx3d_*` builtins provide a simple 3D mode (cube, grid, model
  loading) — see `examples/native_3d.npp`.

## Limits

Graphics requires the SDL2 runtime. On machines without SDL the
`engine2d_init`/`gfx_init` call returns false and `gfx_error()`
describes the failure. The built-in native backend supports a documented
subset; full graphics usually means building with `--backend llvm`
(Clang-compatible compiler required).
