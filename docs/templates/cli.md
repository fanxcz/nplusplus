# CLI Template

The `cli` template is a tiny interactive command-line starter. The checked-in `examples/templates/cli.npp` asks for the user's name and prints a greeting. It does not provide a command parser, subcommands, automatic help text or configuration-file handling.

## Create and run

```bash
npp new MyCli --template cli
cd MyCli
npp check
npp build
npp run
```

## Included program

```npp
fn main() {
    print("N++ application")
    let name = input("Your name: ")
    print("Hello, " + name)
}
```

## Add arguments

The runtime currently exposes `os.args_count()` and `os.args_get(index)` built-ins, wrapped by `std.os` as `args_count()` and `args_get(index)`. For prompted input, `std.io` contains `read_line(prompt)`, `read_i64(prompt)` and `read_f64(prompt)`. See [std.os](../std/os.md) and [std.io](../std/io.md).

The generated project is a starter source file. Add command routing, validation, tests and error handling explicitly as the application grows.
