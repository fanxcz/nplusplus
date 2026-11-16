# `std.base64`

`stdlib/base64.npp` implements RFC 4648 base64 encoding and decoding on top
of `string.byte_at`, `string.from_char` and `string.slice`, so it runs on the
builtin native backend.

| Function | Description |
|---|---|
| `base64_encode(data: string) -> string` | Encode bytes to base64 with `=` padding |
| `base64_decode(data: string) -> string` | Decode base64; returns `""` on invalid input |
| `base64_valid(data: string) -> bool` | Round-trip sanity check for an encoding |

## Example

```npp
import std.base64

fn main() {
    let enc = base64_encode("hello")
    print(enc)                    // aGVsbG8=
    print(base64_decode(enc))     // hello
}
```
