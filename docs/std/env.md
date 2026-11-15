# `std.env`

`stdlib/env.npp` reads process environment variables. Missing variables
read as empty strings; `env_has` distinguishes unset from empty.

| Function | Description |
|---|---|
| `env_get(name: string) -> string` | Raw value of `name`, or `""` |
| `env_has(name: string) -> bool` | `true` when the variable is set and non-empty |
| `env_int(name: string, fallback: i64) -> i64` | Parsed base-10 integer, `fallback` when unset/malformed |

## Example

```npp
import std.env

fn main() {
    if env_has("HOME") {
        print("HOME=" + env_get("HOME"))
    }
    let workers = env_int("NPP_WORKERS", 4)
    print("workers: " + str_i64(workers))
}
```

`env_int` accepts an optional leading `-` and ASCII digits only; values like
`12.5`, `0x10` or empty strings return the fallback.
