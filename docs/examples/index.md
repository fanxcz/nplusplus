# Examples

The repository's `examples/` directory contains standalone `.npp` programs and assets. These are runnable source examples, not examples fetched from a package registry.

## In This Section

| Topic | Description |
|---|---|
| [Overview](overview.md) | Index of examples in the source tree |
| [Native app sample](https://github.com/fanxcz/nplusplus/blob/main/examples/native_app.npp) | Minimal native window and interactive drawing sample |
| [Project templates](../templates/index.md) | Starter projects for applications, CLI tools, games, and libraries |
| [Games](games.md) | Terminal and graphical game prototypes |
| [Graphics](graphics.md) | Valid 2D, 3D and drawing examples |
| [Audio](audio.md) | The current sound-runtime demo |
| [Application UI Playground](https://github.com/fanxcz/nplusplus/blob/main/examples/ui_playground.npp) | Immediate-mode UI widgets and layout |
| [2D Starter](https://github.com/fanxcz/nplusplus/blob/main/examples/game2d_starter.npp) | Asset-free tilemap/physics/camera/animation demonstration |

## Run an example

From the repository root (or any environment where the compiler can find the source tree and runtime), pass the source path directly:

```bash
npp check examples/hello.npp
npp run examples/hello.npp
```

There is no `npp run --example NAME` option in the current CLI. For a project scaffold, use `npp new MyProject --template app` and then run the project commands from its directory.

Graphical examples need a graphics-capable session and the required runtime libraries. Audio and resource demos may require optional host dependencies and the working directory to remain the repository root so relative asset paths resolve.
