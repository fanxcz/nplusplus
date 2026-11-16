# `std.map`

`stdlib/map.npp` provides `IntMap`, an open-addressing hash map from `i64`
keys to `i64` values built on `ilist` handles. It uses linear probing with
tombstones and doubles capacity at 70% load, giving O(1) average operations.
Keys and values are `i64` only — it is not a generic map.

All functions are native-backend safe (they use only `ilist_*` builtins).

| Function | Description |
|---|---|
| `map_new() -> IntMap` | Create an empty map (initial capacity 16) |
| `map_set(m, key, value) -> IntMap` | Insert or update; returns the map (may be rehashed) |
| `map_get(m, key, out: *i64) -> bool` | Fetch value into `out`; false when absent |
| `map_contains(m, key) -> bool` | Membership test |
| `map_remove(m, key) -> bool` | Delete key; false when absent |
| `map_len(m) -> i64` | Number of live entries |
| `map_clear(m)` | Remove all entries, keep capacity |
| `map_free(m)` | Release the underlying lists |

`IntMap` is a plain struct of four `ilist` handles and is passed by value;
mutations are visible through the shared handles, so callers must assign
the result of `map_set` back: `m = map_set(m, k, v)`.

## Example

```npp
import std.map

fn main() {
    let mut m = map_new()
    m = map_set(m, 1, 100)
    m = map_set(m, 2, 200)
    let mut out: i64 = 0
    if map_get(m, 2, &out) {
        print(out)               // 200
    }
    print(map_len(m))            // 2
    map_free(m)
}
```
