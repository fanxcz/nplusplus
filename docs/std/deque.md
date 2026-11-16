# `std.deque`

`stdlib/deque.npp` provides a double-ended queue of `i64` values over
`ilist` handles. `push_back`/`pop_back`/`front`/`back` are O(1);
`push_front`/`pop_front` are O(n) because they shift elements inside the
list — prefer `ilist` directly when both ends are not needed.

All functions are native-backend safe.

| Function | Description |
|---|---|
| `deque_new() -> i64` | Create an empty deque (an `ilist` handle) |
| `deque_push_back(d, value) -> bool` | Append at the back |
| `deque_push_front(d, value) -> bool` | Insert at the front (O(n)) |
| `deque_pop_back(d, out: *i64) -> bool` | Remove and return the back |
| `deque_pop_front(d, out: *i64) -> bool` | Remove and return the front (O(n)) |
| `deque_front(d, out: *i64) -> bool` | Read the front without removing |
| `deque_back(d, out: *i64) -> bool` | Read the back without removing |
| `deque_get(d, index, out: *i64) -> bool` | Indexed read (bounds-checked) |
| `deque_set(d, index, value) -> bool` | Indexed write (bounds-checked) |
| `deque_len(d) -> i64` | Number of elements |
| `deque_clear(d) -> bool` | Remove all elements |
| `deque_free(d) -> bool` | Release the list |

## Example

```npp
import std.deque

fn main() {
    let d = deque_new()
    deque_push_back(d, 1)
    deque_push_front(d, 2)
    deque_push_back(d, 3)      // [2, 1, 3]
    let mut v: i64 = 0
    deque_front(d, &v)
    print(v)                   // 2
    deque_pop_front(d, &v)
    print(v)                   // 2
    deque_back(d, &v)
    print(v)                   // 3
    deque_free(d)
}
```
