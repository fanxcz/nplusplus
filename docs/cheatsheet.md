# N++ Cheat Sheet

## CLI commands

| Command | Purpose |
|---|---|
| `npp run file.npp` | Compile and run (auto backend) |
| `npp build file.npp -o app` | Compile to executable |
| `npp check file.npp` | Type-check only, no codegen |
| `npp fmt file.npp [--check]` | Format / verify formatting |
| `npp lint file.npp` | Lint a single file |
| `npp doc` | Generate documentation |
| `npp bench file.npp` | Benchmark a program |
| `npp test` | Run project tests |
| `npp doctor [--json]` | Diagnose toolchain |
| `npp self-test` | Verify native backend end-to-end |
| `npp new Name --template game` | Scaffold a project |
| `npp pkg <init\|add\|list\|build>` | Package management |
| `npp lsp` | Language server (used by editors) |
| `npp repl` | Interactive REPL |

Backends: `--backend auto|native|llvm`. `native` is the built-in x86-64
backend (no external compiler, documented subset); `llvm` needs a
clang-compatible `clang++` on PATH or `NPP_CXX`.

## Language essentials

```npp
fn main() {
    let x: i64 = 42              // immutable
    let mut y: i64 = 0           // mutable
    let s = "text" + str_i64(x)  // string concat via str_i64
    if x > 0 { print("pos") }
    while y < 10 { y = y + 1 }
    for i in 0..10 { print(i) }
    defer cleanup()              // runs on scope exit
}
```

- Integers default to `i64`; floats are `f64`; `bool` is `true`/`false`.
- Pointers: `*T`, address-of `&x`, deref `*p`.
- `Option`/`Result` enums + `?` propagation — see `std.option`, `std.result`.
- Modules: `import std.math` loads `stdlib/math.npp`.

## Frequently used builtins (no import)

| Builtin | Signature |
|---|---|
| `print(value)` | Print with newline |
| `str_i64 / str_f64 / str_bool` | Convert to string |
| `to_i64 / to_f64` | Convert numbers |
| `input(prompt) / input_i64 / input_f64` | Console input |
| `time_ms() / sleep(ms)` | Clock and sleep |
| `random_i64(min, max)` | Random integer |
| `env(name)` | Environment variable |
| `file_read / file_write / file_exists` | Whole-file IO |
| `http_get(url)` | HTTP GET → string |
| `sha256(text)` | SHA-256 hex digest |
| `sqlite_open / sqlite_exec / sqlite_close` | SQLite |
| `key_pressed() / read_key()` | Console keys |
| `exit(code) / beep()` | Process control |
| `string.len / byte_at / chars / char_at / from_char / valid_utf8` | String primitives |
| `string.contains / find / trim / upper / lower / slice / split_count / split_at` | String search/split |
| `ilist_* / slist_*` | Raw growable lists |
| `gfx_*` | Graphics, input, audio, textures, particles |
| `ecs_*`, `scene_*`, `physics_*` | Scene/entity systems |

## stdlib module map

| Need | Module |
|---|---|
| Console prompts | `std.io` |
| Files | `std.fs` |
| Args / env / exit / keys / beep | `std.os`, `std.env`, `std.sys` |
| Text composition | `std.text`, `std.string` |
| Numbers | `std.math`, `std.vec2`, `std.vec3` |
| Time / random | `std.time`, `std.random` |
| Processes | `std.shell` |
| JSON / HTTP / SQLite / SHA-256 | `std.json`, `std.net`, `std.db`, `std.crypto` |
| Lists | `std.collections` (incl. `*_shuffle`, `*_pick`) |
| Option / Result | `std.option`, `std.result` |
| 2D games | `std.engine2d`, `std.tilemap`, `std.physics2d`, `std.sprite2d`, `std.animation2d`, `std.camera2d`, `std.audio2d`, `std.ui` |

Full API tables: [std index](std/index.md).
