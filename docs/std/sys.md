# `std.sys`

`stdlib/sys.npp` wraps process-exit, beep and console key builtins.

| Function | Description |
|---|---|
| `sys_exit(code: i64)` | Exit the process immediately with `code` |
| `sys_beep()` | Emit a short beep (bell character on terminals) |
| `sys_key_pending() -> bool` | True when a key press is waiting in the input buffer |
| `sys_read_key() -> i64` | Block until a key is pressed; returns its character code (`-1` on EOF) |
| `sys_read_key_char() -> string` | Same, decoded as a one-character string (empty on EOF) |

`sys_read_key` returns raw character codes: `119` is `w`, `13` is Enter,
`27` starts escape sequences. Game loops typically poll
`sys_key_pending()` once per frame.

## Example

```npp
import std.sys

fn main() {
    print("Press any key to continue...")
    let code = sys_read_key()
    if code < 0 {
        print("No key available")
    } else {
        print("Got key code: " + str_i64(code))
    }
    sys_beep()
}
```

For named-key handling in graphical games prefer `std.engine2d`
(`engine2d_key_pressed`) or the `gfx_*` builtins, which understand key
names such as `"SPACE"`.
