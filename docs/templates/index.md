# Project Templates

`npp new` and `npp init` seed a project with one checked-in template. The command creates `npp.toml`, standard asset folders, `scenes/main.nxs`, `README.md`, `.gitignore` and `src/main.npp`. It copies one source file; it does not generate a multi-file framework.

## Templates accepted by the current CLI

For the project's primary desktop-application workflow, begin with **App**. It provides a native window and event loop, but it is not yet a full desktop widget framework. Choose CLI for terminal utilities and the other templates for narrower examples.

| Template | Command | Current starting point |
|---|---|---|
| [App](app.md) | `npp new MyApp --template app` | Minimal native window, event loop and 2D primitives |
| [CLI](cli.md) | `npp new MyCli --template cli` | Console input and greeting |
| [Game](game.md) | `npp new MyGame --template game` | Single-file graphical survivor game |
| [3D](3d.md) | `npp new My3D --template 3d` | Grid and coloured cubes |
| [Paint](paint.md) | `npp new MyPaint --template paint` | Raster canvas and BMP tools |
| [Server](server.md) | `npp new MyServer --template server` | HTTP GET client demo, not a server |
| [Library](library.md) | `npp new MyLibrary --template library` | Single-file `add` function example |

An experimental scene-editor source exists at `examples/templates/editor.npp`, but `editor` is **not** accepted by `npp new --template`. See [Editor source](editor.md) for a manual way to try it.

## Create a project

```bash
npp new MyProject --template app
cd MyProject
npp check
npp build
npp run
```

To initialize the current directory, use `npp init --template app`.

## Limitations

Templates are starter files, not complete frameworks. Features such as HTTP listeners, a source-code IDE, advanced physics, scene graphs or asset pipelines are not included unless the source actually implements them. See each template source under `examples/templates/`.
