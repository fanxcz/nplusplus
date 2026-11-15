# Build system

N++ has two different build workflows: building a N++ project with the `npp` CLI, and compiling the N++ compiler itself from C++ source.

## Build a N++ project

From a directory containing a N++ project:

```sh
npp check
npp build
npp run
```

For an optimized project build, use `npp build --release`. The compiler's automatic backend selection uses the built-in native backend when the source fits its supported subset and otherwise tries the configured LLVM/C++ toolchain.

## Build the N++ compiler

The compiler source is built with CMake and a C++20 compiler. A Ninja-based setup is documented in [CMake integration](cmake.md); run the project's CTest suite with the commands in [Building from source](../development/building.md).

## More details

- [CMake configuration and presets](cmake.md)
- [Cross-compilation and backend limits](cross-compilation.md)
- [Targets and actual backend support](targets.md)
- [Built-in native backend subset](../tooling/native-backend.md)
- [CLI command reference](../tooling/commands.md)
