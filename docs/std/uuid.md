# `std.uuid`

`stdlib/uuid.npp` generates RFC 4122 version 4 UUIDs from `random_i64` and
validates the canonical 8-4-4-4-12 hexadecimal form. It is pure N++ and runs
on the builtin native backend.

| Function | Description |
|---|---|
| `uuid_v4() -> string` | Random version-4 UUID, e.g. `f47ac10b-58cc-4372-a567-0e02b2c3d479` |
| `uuid_valid(text: string) -> bool` | Check length, dash positions and hex digits |

## Example

```npp
import std.uuid

fn main() {
    let id = uuid_v4()
    print(id)
    print(uuid_valid(id))   // true
}
```
