# `std.time`

`stdlib/time.npp` wraps the runtime clock and sleep builtins. Timestamps are
milliseconds from an unspecified epoch; only differences are meaningful.

| Function | Description |
|---|---|
| `time_now_ms() -> i64` | Current timestamp in milliseconds |
| `time_sleep_ms(ms: i64)` | Sleep for `ms` milliseconds |
| `time_elapsed_ms(start_ms: i64) -> i64` | Milliseconds since `start_ms` |
| `time_seconds() -> f64` | Current timestamp in seconds |
| `time_seconds_elapsed(start_ms: i64) -> f64` | Seconds since `start_ms` |

## Example

```npp
import std.time

fn main() {
    let start = time_now_ms()
    time_sleep_ms(250)
    print("elapsed ms: " + str_i64(time_elapsed_ms(start)))
    print("elapsed s:  " + str_f64(time_seconds_elapsed(start)))
}
```

These helpers are not part of the built-in native subset; use the LLVM
backend (`--backend llvm`, the default for `auto` when the subset does not
apply).
