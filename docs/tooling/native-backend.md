# Builtin native backend

N++ ships a builtin backend that translates supported programs straight
into runnable machine code. No LLVM, Clang, assembler, linker, SDK, or other
development tool is involved at any point, so these programs compile on a
bare machine with only the `npp` binary next to its standard library.

```sh
npp build app.npp -o app --backend native
npp run app.npp --backend native
```

With the default `--backend auto`, N++ uses the builtin backend whenever
the program fits the subset below and otherwise falls back to the LLVM
backend (which needs a C++ compiler: `clang++` on `PATH`, `NPP_CXX`, or a
bundled wrapper — see `npp doctor`).

## Targets

| Target | Output | Status |
|---|---|---|
| `x86_64-windows` | PE32+ console `.exe` (imports resolved by the OS loader) | supported |
| `x86_64-linux` | ELF64 executable (direct syscalls only) | supported |
| `native` | host default on x86-64 Windows/Linux | supported |
| `aarch64-*`, `macos`, `wasm32-wasi`, `android` | — | LLVM backend only (Mach-O/AArch64 emitters are future work) |

`npp targets` lists the LLVM targets and the builtin coverage.
`--backend native` with an unsupported target fails with an explicit
`has no emitter for target` error instead of producing a broken binary.

## Supported subset (v0)

- Types: `i64`, `bool`, `string`, `f64`, `void`, fixed-size arrays
  (`[T; n]` syntax in annotations, `[a, b, c]` literals), structs whose
  fields are themselves supported types (including nesting), and pointers
  (`*T`, `&x`, `*p` dereference and `*p = v`) used for output parameters
  and raw memory access.
- Functions with `i64`/`bool`/`string`/`f64`/array/struct parameters and
  results, including recursion; `fn main()` (returning nothing or `i64`
  exit code) is required.
- Statements: `let`/`mut`, assignment (`=`, `+=`, …), `if`/`else`,
  `while`, `for i in a..b`, `for x in array` (iterates element copies),
  `break`, `continue`, `return`, member assignment (`p.field = v`),
  index assignment (`xs[i] = v`), and compound assignment on lvalues
  (`x += v`, `p.f -= v`, `xs[i] *= v`, including `string +=`), and
  `defer call()` at function top level (runs in reverse order at return).
- Expressions: integer `+ - * / %`, floating-point `+ - * /` on `f64`,
  unary minus, comparisons (integer and floating-point), boolean
  `&& || !`, string concatenation (`+`), string `==`/`!=`, calls,
  array literals, indexing (`xs[i]`), member access (`p.field`),
  struct literals (`P { x: 1 }`), and `len(array)` (compile-time size).
- Aggregates are arena-allocated blocks of 8-byte slots and behave like
  values: `let`/`=`/call arguments deep-copy them (nested aggregates
  included), so mutating a copy never aliases the original. Indexing is
  bounds-checked and aborts with `index out of bounds` on stderr.
- I/O: `print(i64 | bool | string | f64)` (trailing newline, like the
  runtime; `f64` prints shortest-round-trip decimal, fixed notation for
  decimal exponents in `[-4, 14]` and scientific notation otherwise),
  `input(prompt)` and `input_i64(prompt)`/`input_f64(prompt)` (one stdin
  line, no trailing newline; invalid input re-prompts on stderr),
  `str_i64(i64)`, `str_bool(bool)`, `str_f64(f64)`, `to_f64(i64)`,
  `to_i64(f64)`.
- System: `exit(code)`, `time_ms()` (monotonic milliseconds),
  `sleep(ms)`, `random_i64(min, max)` (SplitMix64, unbiased over the full
  `i64` range, swapped bounds like the runtime), `file_exists(path)`,
  `file_read(path)`, `file_read_checked(path, *i64)` /
  `file_write(path, data)`, `mem_alloc`/`mem_read`/`mem_set`/`mem_copy`/
  `mem_free`, `os.args_count()`, `os.args_get(i)`, `env(name)`,
  `beep(freq, ms)`, `key_pressed(code)`, `read_key()`,
  `string.len(s)`, `string.byte_at(s, i)`, `string.char_at(s, i)`.
- Strings (UTF-8 aware): `string.len`, `string.byte_at`,
  `string.char_at`, `string.chars`, `string.contains`, `string.find`,
  `string.slice`, `string.split_at`, `string.split_count`,
  `string.trim`, `string.upper`, `string.lower`, `string.from_char`,
  `string.valid_utf8`.
- Math (`f64`): `sin`, `cos`, `tan`, `sqrt`, `floor`, `ceil`, `atan2`,
  `pow` (x87 FPU), plus `physics_aabb`/`physics_circle` collision
  helpers (SSE2) used by `std.physics2d`/`std.collision2d`.
- Collections: `ilist_*` (integer lists) and `slist_*` (string lists)
  handles with out-parameter `*T` results, backing `std.collections`.
- Standard library: the following `std.*` modules compile and run fully
  natively — `io`, `os`, `env`, `fs`, `sys`, `string`, `text`, `math`,
  `random`, `time`, `memory`, `collections` (with `defer` cleanup),
  `physics2d`, `collision2d`, `tilemap`, `vec2`, `vec3`, `animation2d`,
  `option`, `result`.
- Memory: strings live in a 64 MiB bump arena reserved at startup and
  released at process exit. Exhausting it ends the program with a clean
  `out of string memory` error instead of corrupting memory. There is no
  per-allocation heap overhead and no leak across runs.

- Read-only slices `[]T`: `values[start..end]` produces a non-owning
  `{data, len}` view (16-byte arena header), with unsigned bounds
  checks, indexing, `len`, nested slicing, parameters/returns, and
  pass-through call arguments. Mutable slices `mut []T` remain
  LLVM-only.

- Enums and `match`: payload enums lower to a `{tag, payload}` arena
  block (payload travels as raw bits — f64 via `movq`, aggregates as
  pointers). `match` supports variant patterns with payload bindings,
  wildcard/catch-all arms, bare `v =>` bindings, and `if` guards;
  non-enum match values, range patterns, and `mut []T` stay LLVM-only.
  This unblocks the `std.option` and `std.result` modules natively.

Everything else (mutable slices, non-enum `match`, range patterns,
JSON/SHA-256, networking,
graphics/audio/UI/ECS/database builtins, `extern`, `system()`) is
rejected with a precise `outside the builtin native v0 subset` reason
naming the construct and function. The LLVM-only stdlib modules are
`json`, `crypto`, `net`, `db`, `shell`,
`audio2d`, `engine2d`, `sprite2d`, `ui`, `camera2d`, `tilemap_render`.
With `--backend auto` that reason appears only when the LLVM fallback
also cannot proceed; `--backend native` surfaces it directly.

## Verification

- `tests/native_programs/*.npp` are built with `--backend native`, executed,
  and byte-compared against expected output (`ctest -R npp_native_smoke`).
- The same programs built with `--backend llvm` must produce identical
  output (checked for the core battery during development).
- Emitted images are validated with LLVM `readobj`/`objdump`, and every
  machine-code emitter change is re-checked by disassembly.
- The Windows CI job builds and runs the battery with compilers removed
  from `PATH`; the Linux job additionally executes the ELF output.
