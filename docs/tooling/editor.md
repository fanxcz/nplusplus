# Editor Integration

N++ has two editor-related workflows. The `editor` command generates workspace files; the graphical editor template is experimental.

## Generate VS Code project settings

From a N++ project directory:

```sh
npp editor
```

This writes:

- `.vscode/settings.json` — associates `.npp` files with the `npp` language ID and sets indentation defaults.
- `.vscode/tasks.json` — adds a basic `N++ Build` task which runs `npp build --release`.

Install the [N++ VS Code language extension](vscode-extension.md) to get syntax colors, bracket behavior, snippets, and access to the N++ Language Server. The extension is source-only in this repository and is not currently on the VS Code Marketplace.

## Graphical editor example

```sh
npp editor --gui
```

This experimental path compiles and launches the bundled editor example. It can require the LLVM/Clang fallback and graphics runtime, depending on which constructs are used and how N++ was packaged. It is not yet a full replacement for a mature IDE.

## Language server

The extension starts `npp lsp` for live compiler diagnostics, completion, signature help, hover, go-to-definition, references, rename, document/range formatting, quick fixes, and workspace-symbol search. Workspace indexing uses lexical declarations and known inferred types rather than a complete semantic model; import aliases and some cross-file field inference remain limited. A source debugger/debug adapter and format-on-save workflow are not included.

## Limits

The CLI has no command to open a specific file at a line, pipe-to-editor mode, or configurable theme/keybinding system. Consult the [CLI command reference](commands.md) for supported commands and the [roadmap](../development/roadmap.md) for planned improvements.
