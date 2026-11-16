# Backend Support Matrix

This matrix records evidence in the source tree at the current baseline. "Supported" means explicit subset documentation and/or executable regression coverage exists. "Not supported" means the builtin native backend rejects the construct. "Needs verification" is used where parsing or LLVM declarations alone do not prove end-to-end behavior for all APIs or targets.

## Language constructs

| Feature | Builtin native | LLVM path | Evidence / notes |
|---|---|---|---|
| `i64`, `bool`, `string`, `f64`, `void` | Supported | Supported | Native subset declaration and native regression fixtures |
| Functions, recursion, locals, assignment | Supported in documented subset | Supported | Native subset and regression tests |
| `if/else`, `while`, range `for`, collection `for`, `break`, `continue`, `return` | Supported in documented subset | Supported | Native fixtures and compiler regression suite |
| Integer arithmetic, comparisons, boolean operators, calls, string concatenation/equality | Supported in documented subset | Supported | Native fixtures and parity test |
| `print`, `input`, `input_i64`, `input_f64`, `str_i64`, `str_bool`, `str_f64`, `to_f64`, `to_i64` | Supported in documented signatures | Supported | Native emitter/runtime subset |
| `exit`, `time_ms`, `sleep`, `random_i64`, `file_exists`, `os.args_*`, `string.len`, `string.byte_at` | Supported | Supported | Native fixtures (`sys_io.npp`, `input_i64.npp`, `exit_builtin.npp`) |
| i64 bitwise and shift operators | Supported | Supported | Native and LLVM parity fixtures; 2.2.0 changelog |
| Fixed-width integers (`i8/i16/i32`, `u8/u16/u32/u64`) and `f32` | Not supported | Supported for documented scalar lowering paths | 1.0.0 changelog; native subset excludes these types |
| `f64` and floating-point expressions | Supported (SSE2 scalar ops, `f64` params/returns, formatting/parsing) | Supported for documented builtins/expressions | Native fixtures (`f64.npp`, `input_f64.npp`); LLVM math tests exist |
| Fixed arrays | Supported (arena blocks, bounds-checked indexing, `len`, copy semantics) | Supported for tested declarations, indexing and aggregate paths | Native fixtures (`arrays.npp`); regression tests exist |
| Read-only slices `[]T` | Supported (non-owning `{data,len}` view, bounds-checked indexing, `len`, nesting, params/returns) | Supported for tested slice expressions, indexing, parameters and returns | Native fixture `tests/native_programs/slices.npp`; `tests/slice_llvm_smoke.cmake` |
| Mutable slices `mut []T` | Not supported | Supported for documented lexical exclusive-borrow subset | Unreleased changelog and `tests/mutable_slice_smoke.cmake` |
| Structs (fields, member access/assignment, params/returns, nesting) | Supported (arena blocks, deep-copy value semantics) | Supported for tested declaration, construction and access paths | Native fixtures (`structs.npp`); regression tests exist |
| Enum payloads | Not supported | Supported for tested declaration, construction and access paths | Native subset excludes enums |
| `match` and exhaustiveness over bool/enum | Not supported | Supported for tested finite domains | Match smoke tests and 2.2.0 changelog |
| Pointer operations (`*T`, `&x`, dereference) | Supported for raw memory/output parameters | Needs verification per operation and lifetime pattern | Native subset supports pointers for `mem_*`/out-params; borrow/lifetime analysis stays on the LLVM path |
| `extern` declarations / FFI | Not supported | Needs verification | Native subset explicitly rejects `extern`; inspect target ABI and runtime tests |
| Standard library: io, os, env, fs, sys, string, text, math, random, time, memory, collections, physics2d, collision2d, tilemap, vec2, vec3, animation2d | Supported (native builtins + pure-N++ modules) | Supported | Module-import smoke check with `--backend native`; collection fixtures |
| Standard library: json, crypto, net, db, shell, option, result, audio2d, engine2d, sprite2d, ui, camera2d, tilemap_render | Not supported as native builtins | Needs verification per module/API and target | Runtime implementations exist; native subset documents only a smaller builtin surface |
| Graphics, audio, UI, ECS and other SDL-dependent APIs | Not supported as native builtins | Needs verification per API and host | Runtime sources exist; SDL2 is optional and not every API has cross-platform test coverage |
| Project/package commands | Driver functionality, not generated-code backend support | Driver functionality, not generated-code backend support | See project and package docs |

## Output targets

| Target | Builtin native | LLVM path |
|---|---|---|
| `x86_64-windows` | Supported (PE32+) | Depends on configured compatible compiler/toolchain |
| `x86_64-linux` | Supported (ELF64) | Depends on configured compatible compiler/toolchain |
| `x86_64-macos`, `aarch64-*`, `wasm32-wasi`, `android` | Not supported by builtin emitter | Needs verification for a concrete host toolchain, target sysroot, ABI and runtime |

## Updating this table

Every promoted feature needs a focused source test and, where parity is expected, a backend-specific executable test. Change "Needs verification" only after testing the complete code-generation and runtime path for the claimed target. See [builtin native backend](tooling/native-backend.md), [backend explainer](tooling/backend-explainer.md), and [platform support](development/platform-support.md).
