# `std.text`

`stdlib/text.npp` adds composition helpers on top of the `string.*` builtins:
case conversion, trimming, searching, padding, repeating and joining.

| Function | Description |
|---|---|
| `text_len(s) -> i64` | Length in bytes (alias of `string.len`) |
| `text_upper(s) -> string` | ASCII uppercase |
| `text_lower(s) -> string` | ASCII lowercase |
| `text_trim(s) -> string` | Strip leading/trailing whitespace |
| `text_contains(s, part) -> bool` | Substring search |
| `text_find(s, part) -> i64` | Byte index of `part`, `-1` when absent |
| `text_slice(s, start, len) -> string` | Substring by byte range |
| `text_split_count(s, sep) -> i64` | Number of separator-separated fields |
| `text_split_at(s, sep, index) -> string` | Field `index` (0-based) after splitting |
| `text_starts_with(s, prefix) -> bool` | Prefix check |
| `text_ends_with(s, suffix) -> bool` | Suffix check |
| `text_repeat(s, count) -> string` | Concatenate `s` `count` times |
| `text_pad_left(s, width, fill) -> string` | Left-pad to `width` bytes |
| `text_pad_right(s, width, fill) -> string` | Right-pad to `width` bytes |
| `text_join(list, sep) -> string` | Join a `std.collections` string list |

## Example

```npp
import std.collections
import std.text

fn main() {
    print(text_pad_left("42", 5, "0"))      // 00042
    print(text_starts_with("main.npp", "main"))

    let words = string_list_new()
    defer string_list_dispose(words)
    string_list_push(words, "alpha")
    string_list_push(words, "beta")
    print(text_join(words, ", "))            // alpha, beta
}
```

`text_join` takes a handle produced by the `std.collections` `string_list_*`
helpers. Padding counts bytes, not display columns; multi-byte UTF-8 text may
need manual care.
