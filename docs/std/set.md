# `std.set`

`stdlib/set.npp` implements a set of `i64` values on top of `IntMap`
(`std.map`), storing each element as `key -> key`. Membership, insert and
remove are O(1) average; iteration order is unspecified.

| Function | Description |
|---|---|
| `int_set_new() -> IntMap` | Create an empty set |
| `int_set_add(s, value) -> IntMap` | Insert value; returns the set (may be rehashed) |
| `int_set_contains(s, value) -> bool` | Membership test |
| `int_set_remove(s, value) -> bool` | Delete value; false when absent |
| `int_set_len(s) -> i64` | Number of elements |
| `int_set_clear(s)` | Remove all elements, keep capacity |
| `int_set_free(s)` | Release the underlying lists |

Like `std.map`, assign the result of `int_set_add` back:
`s = int_set_add(s, v)`.

## Example

```npp
import std.set

fn main() {
    let mut s = int_set_new()
    s = int_set_add(s, 10)
    s = int_set_add(s, 20)
    s = int_set_add(s, 10)      // duplicate ignored
    print(int_set_len(s))       // 2
    print(int_set_contains(s, 20))  // true
    int_set_free(s)
}
```
