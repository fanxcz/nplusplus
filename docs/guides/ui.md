# Guide: immediate-mode UI

`std.ui` provides retained-state helpers on top of the `ui_*` builtins,
and `UIRect` layout helpers for rows, columns and grids.

## Rectangles first

```npp
import std.ui

fn main() {
    let panel = ui2d_rect(40.0, 40.0, 320.0, 200.0)
    ui2d_panel(panel, 0.12, 0.14, 0.18, 0.9)
    ui2d_outline(panel, 2.0, 0.4, 0.5, 0.9, 1.0)

    // Layout: three buttons in a row inside the panel.
    let inner = ui2d_inset(panel, 12.0)
    let b0 = ui2d_row_cell(inner, 0, 3, 8.0)
    let b1 = ui2d_row_cell(inner, 1, 3, 8.0)
    let b2 = ui2d_row_cell(inner, 2, 3, 8.0)

    ui2d_button(b0, "Start")
    ui2d_button(b1, "Options")
    ui2d_button(b2, "Quit")
}
```

`ui2d_row_cell(parent, index, count, gap)` divides `parent`
horizontally; `ui2d_column_cell` vertically; `ui2d_grid_cell`
by column/row. `ui2d_contains(rect, px, py)` is a hit test.

## Widgets

| Function | Returns |
|---|---|
| `ui2d_label(x, y, text, scale)` | draws text |
| `ui2d_button(rect, label) -> bool` | true on click |
| `ui2d_checkbox(x, y, label, checked) -> bool` | toggled state |
| `ui2d_slider(rect, value) -> f64` | dragged value |
| `ui2d_progress(rect, value, r,g,b,a)` | progress bar |
| `ui2d_text_field(rect, value, placeholder, max_bytes) -> string` | edited text |
| `ui2d_separator(x1,y1,x2,y2,thickness,...)` | divider line |

## Inside a game loop

UI widgets must be called between `engine2d_begin_frame` and
`engine2d_end_frame` — they draw immediately and read mouse state at
call time. Keep widget state (checkbox value, text field content) in
your own variables and pass it in each frame; the API is
immediate-mode, not retained.

```npp
let mut volume: f64 = 0.5
let mut name: string = ""

while !engine2d_should_close() {
    engine2d_begin_frame(0.0, 0.0, 0.0, 1.0)
    volume = ui2d_slider(volume_rect, volume)
    name = ui2d_text_field(name_rect, name, "player", 16)
    engine2d_end_frame()
}
```

See `examples/ui_playground.npp` for a complete demo and
`docs/std/ui.md` for the full function table.
