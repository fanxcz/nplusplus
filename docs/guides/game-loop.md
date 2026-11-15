# Guide: game loop

The canonical N++ game structure is a fixed-order loop:

1. `engine2d_init(width, height, title)` — open window/renderer.
2. Loop while `!engine2d_should_close()`:
   - `engine2d_begin_frame(r, g, b, a)` — poll events, clear screen.
   - Read input (`engine2d_key_down`, `engine2d_key_pressed`,
     `engine2d_mouse_*`).
   - Update simulation with `engine2d_delta_time()`.
   - Draw sprites/shapes/text.
   - `engine2d_end_frame()` — present.
3. `engine2d_shutdown()`.

## Skeleton

```npp
import std.engine2d
import std.time

fn main() {
    if !engine2d_init(800, 600, "My Game") { return }
    engine2d_vsync(true)

    let mut x: f64 = 100.0
    let mut score: i64 = 0

    while !engine2d_should_close() && !engine2d_key_down("ESC") {
        engine2d_begin_frame(0.05, 0.06, 0.09, 1.0)
        let dt = engine2d_delta_time()

        if engine2d_key_down("D") { x = x + 240.0 * dt }
        if engine2d_key_down("A") { x = x - 240.0 * dt }
        if engine2d_key_pressed("SPACE") { score = score + 1 }

        engine2d_draw_rect(x, 300.0, 48.0, 48.0, 0.9, 0.4, 0.2, 1.0)
        engine2d_draw_text(20.0, 20.0, 2.0, "Score: " + str_i64(score),
                           1.0, 1.0, 1.0, 1.0)
        engine2d_end_frame()
    }
    engine2d_shutdown()
}
```

## Timing

- `engine2d_delta_time()` returns seconds since the previous frame —
  multiply velocities by it for frame-rate-independent movement.
- `engine2d_vsync(true)` caps the frame rate to the display refresh.
- `engine2d_set_fps(n)` requests an FPS cap when vsync is off.

## Input

| Helper | Meaning |
|---|---|
| `engine2d_key_down("A")` | Key held this frame |
| `engine2d_key_pressed("SPACE")` | Key went down this frame (edge) |
| `engine2d_mouse_x/y()` | Cursor position |
| `engine2d_mouse_down(0)` | Mouse button held (0=left) |
| `engine2d_mouse_clicked(0)` | Mouse button clicked this frame |
| `engine2d_mouse_over(x,y,w,h)` | Cursor inside a rectangle |
| `engine2d_mouse_clicked_in(x,y,w,h)` | Click inside a rectangle |

Key names are uppercase strings: `"A"`–`"Z"`, `"SPACE"`, `"ENTER"`,
`"ESC"`, `"UP"`, `"LEFT"`, digits, `"F1"`–`"F12"`.

## Physics and tilemaps

For platformers use `std.physics2d` (gravity, jump, friction,
tilemap collision) + `std.tilemap` + `std.tilemap_render` —
`examples/game2d_starter.npp` is a complete worked example.

## Console games

Without a window, `examples/terminal_game.npp` and
`examples/npp_studio.npp` show the `screen_*` + `key_pressed()` /
`read_key()` loop used for terminal UIs.
