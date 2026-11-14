# Game Examples

The repository includes several games/prototypes. They range from terminal simulations to small native graphical demonstrations; none should be interpreted as a production or multiplayer engine unless the source actually implements that scope.

## Neon Survivor

`examples/neon_survivor.npp` is the game template source. It has a start screen, arena movement, mouse aiming/shooting, basic enemies, projectile slots, score/wave/health state, pause and game-over flow.

```bash
npp check examples/neon_survivor.npp
npp run examples/neon_survivor.npp
```

## Terminal Arena

`examples/npp_arena.npp` is a text-mode turn-based hero-versus-boss simulation with HP/mana/levels, actions, rounds and score/rank values. It is a terminal program, not a real-time multiplayer arena shooter.

```bash
npp run examples/npp_arena.npp
```

## Terminal movement game

`examples/terminal_game.npp` draws a simple `@` player in a terminal grid, accepts WASD movement and Q to quit, and updates a counter. It depends on terminal input behavior, which can vary between shells/hosts.

## Small graphical game

`examples/native_graphics_game.npp` demonstrates 2D movement, a mouse-controlled target, score display and basic primitive drawing. See [Graphics Examples](graphics.md).

## Vector Lab

[`examples/vector_lab.npp`](https://github.com/fanxcz/nplusplus/blob/main/examples/vector_lab.npp) is a small playable arena sample using the `std.vec2`, `std.collision2d` and `std.animation2d` modules. Move with WASD or arrow keys, slide along obstacles, and collect the pulsing beacon. It demonstrates delta-time movement, vector normalization, circle-vs-rectangle collision and a ping-pong animation curve.

```bash
npp check examples/vector_lab.npp
npp run examples/vector_lab.npp
```

## Tilemap platformer

[`examples/game2d_platformer.npp`](https://github.com/fanxcz/nplusplus/blob/main/examples/game2d_platformer.npp) is a no-external-assets platformer demo using `std.tilemap`: grid-based platforms, horizontal/vertical collision resolution, gravity, jumping and a simple colored tile renderer. The tilemap API is documented under [2D Game Helpers and Tilemaps](../runtime/game2d.md).

## Other application demos

- `examples/npp_focus.npp`: graphical focus/break timer with mouse buttons, sound tone and a small session-count file.
- `examples/npp_editor.npp`: experimental 2D scene/box editor built on the current ECS, UI and snapshot helpers.
- `examples/npp_studio.npp`: terminal-menu demo with calculator, guessing-game and mini-battle routines.
- `examples/npp_paint.npp`: native raster painting tool; documented under [Paint Template](../templates/paint.md).

These examples are meant to show what the compiler/runtime currently supports. Check source and backend requirements before relying on a capability in your own application.
