# Installation

## Install from a release

Pre-built archives for Windows, Linux and macOS are attached to
[GitHub Releases](https://github.com/fanxcz/nplusplus/releases). Each archive
contains the `npp` compiler plus the runtime, standard library and project
templates, with a `.sha256` checksum sidecar.

One-line installers download the latest release, verify the checksum and
place the tools on disk:

```bash
# Linux / macOS
curl -fsSL https://raw.githubusercontent.com/fanxcz/nplusplus/main/scripts/install.sh | bash
```

```powershell
# Windows (PowerShell)
irm https://raw.githubusercontent.com/fanxcz/nplusplus/main/scripts/install.ps1 | iex
```

Set `NPP_VERSION` to pin a specific tag and `NPP_PREFIX` to choose the install
root (default `~/.local/npp` on Unix, `%LOCALAPPDATA%\NPP` on Windows). The
scripts print the `PATH` update to apply afterwards.

To install manually, download the archive for your platform, verify the
`.sha256` sidecar, and extract it so that `bin/npp` and `share/npp/` sit side
by side; then add `bin/` to your `PATH`.

## Building from source

N++ can also be built from source with CMake. The compiler's `CMakeLists.txt` minimum is 3.20, but the checked-in `CMakePresets.json` uses schema version 6, which requires CMake 3.25+. If you have CMake 3.20–3.24, use explicit configure/build commands (`cmake -S . -B build -G Ninja -DNPP_BUILD_TESTS=ON` and `cmake --build build`) instead of `--preset` commands.


### Prerequisites

| Dependency | Minimum Version | Notes |
|------------|----------------|-------|
| CMake | 3.20 | Required for build system |
| Ninja | 1.10 | Recommended generator |
| Clang/LLVM | 18 | Required for LLVM fallback builds; not needed for all built-in native subset programs |
| SDL2 | 2.28 | Optional for building the graphics/audio-enabled runtime |
| C++20 Compiler | Clang 18 / GCC 13 / MSVC 19.40 | |

#### Ubuntu / Debian

```bash
sudo apt-get update && sudo apt-get install -y \
    cmake ninja-build clang-18 lld-18 \
    libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev
```

#### Arch Linux

```bash
sudo pacman -S cmake ninja clang lld sdl2 sdl2_image sdl2_mixer sdl2_ttf
```

#### Fedora

```bash
sudo dnf install cmake ninja-build clang lld SDL2-devel SDL2_image-devel SDL2_mixer-devel SDL2_ttf-devel
```

#### Windows (PowerShell)

```powershell
choco install cmake ninja llvm sdl2
# Or use vcpkg: vcpkg install sdl2 sdl2-image sdl2-mixer sdl2-ttf
```

#### macOS

```bash
brew install cmake ninja llvm sdl2 sdl2_image sdl2_mixer sdl2_ttf
```

### Build Steps

```bash
git clone https://github.com/fanxcz/nplusplus.git
cd nplusplus

cmake --preset=release
cmake --build --preset=release

# Unit, integration, backend, LSP, doctor, and self-test checks
ctest --test-dir build --output-on-failure

# Diagnose the local environment
./build/npp doctor --json

# Install (optional)
sudo cmake --install build
```

### Verify Installation

```bash
npp --version
npp doctor
npp self-test
```

## IDE Integration

### VS Code

Follow the [VS Code language extension guide](../tooling/vscode-extension.md). The project contains the extension source but does not yet publish a Marketplace extension. For basic workspace settings and build task generation, run `npp editor` inside a N++ project. Configure `npp lsp` as the language server for diagnostics and completion.

### CLion

Open the N++ compiler repository and select a supported configure preset from `CMakePresets.json`.

### Neovim / Vim

The N++ language server can be configured to run `npp lsp` using the editor's standard LSP client. Compiler C++ development can use `clangd` after generating a compilation database with CMake.
 
## Troubleshooting

### "Clang not found"

Run `npp doctor` to see whether the LLVM fallback was found. Supported built-in native-subset programs may still compile without a Clang executable. Programs outside the built-in subset require an available compiler via PATH, `NPP_CXX`, or the toolchain supplied by the platform package.

### SDL2 not found

CMake can build the compiler without SDL2, but the graphics/audio runtime will be limited. Install SDL2 development libraries for your platform and reconfigure the project if you need those features.

### Getting Help

- Check [GitHub Issues](https://github.com/fanxcz/nplusplus/issues) for known problems.
- Run `npp doctor --json` for script-friendly environment diagnostics.
- Open a [new issue](https://github.com/fanxcz/nplusplus/issues/new/choose) with a reproducible example.
