# `std.random`

`stdlib/random.npp` wraps the `random_i64` builtin with friendlier helpers.
The generator is not cryptographically secure; do not use it for tokens,
keys or security-sensitive values.

| Function | Description |
|---|---|
| `rand_range(min: i64, max: i64) -> i64` | Inclusive random integer in `[min, max]` |
| `rand_int(max: i64) -> i64` | Random integer in `[0, max]` |
| `rand_f64() -> f64` | Random float in `[0.0, 1.0)` |
| `rand_bool() -> bool` | Random `true`/`false` |
| `rand_chance(percent: i64) -> bool` | `true` with `percent`% probability (1–100) |

## Example

```npp
import std.random

fn main() {
    let roll = rand_range(1, 6)
    print("d6 roll: " + str_i64(roll))

    if rand_chance(50) {
        print("coin flip: heads")
    } else {
        print("coin flip: tails")
    }
}
```
