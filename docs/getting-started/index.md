# Getting Started

Welcome to N++! This section will help you install N++ and create your first project.

## In This Section

| Guide | Description |
|-------|-------------|
| [Installation](installation.md) | Install N++ on your platform |
| [Quick Start](quickstart.md) | Create your first project in 5 minutes |
| [Project Structure](project-structure.md) | Understand the project layout |

## Prerequisites

- **OS**: Linux (x86_64), Windows (x86_64), macOS (x86_64/arm64)
- **For source builds**: C++20 compiler, CMake 3.25+ for the checked-in presets, and Ninja. SDL2 development libraries are needed for graphics/runtime builds.

## Build from source

```bash
sudo apt-get install cmake ninja-build clang-18 lld-18 libsdl2-dev

git clone https://github.com/fanxcz/nplusplus.git
cd nplusplus
cmake --preset=release
cmake --build --preset=release
sudo cmake --install build
```

The compiler's built-in native backend supports a defined language subset without a separate Clang toolchain. Broader LLVM-based builds may need Clang/LLVM, and graphics/runtime features may need SDL2 libraries.

## Verify Installation

```bash
npp --version
npp doctor
```

## Next Steps

1. Follow the [Quick Start](quickstart.md) to create your first project
2. Learn the [Language Basics](../language/overview.md)
3. Explore the [Standard Library](../std/overview.md)
4. Check out [Examples](../examples/overview.md)