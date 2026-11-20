# `std.shell`

`stdlib/shell.npp` wraps process execution builtins. By default commands run
through the platform shell — quote arguments containing spaces or shell
metacharacters, and **never** pass untrusted data to these functions. For
untrusted input use `system_argv` / `capture_output_argv` instead (see
[`std.os`](os.md#shell-free-execution-recommended)), which execute an argv
vector directly with no shell involved.

| Function | Description |
|---|---|
| `shell_run(command: string) -> i64` | Run `command`, return its exit code |
| `shell_output(command: string) -> string` | Run `command`, return captured stdout |
| `shell_ok(command: string) -> bool` | `true` when `command` exits with code 0 |

## Example

```npp
import std.shell

fn main() {
    let rc = shell_run("mkdir build")
    if shell_ok("cd build") {
        print("build dir ready")
    }
}
```

`shell_output` uses `capture_output`, which requires the LLVM backend
(`--backend llvm`). `shell_run`/`shell_ok` map to `system`, which returns the
process exit code on Windows and the raw wait status on POSIX — prefer
`shell_ok` for portability.
