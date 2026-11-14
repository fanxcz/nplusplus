# N++ Language Support — VS Code extension

N++ Language Support registers `.npp` files as the **N++** language. It provides syntax highlighting, snippets and a language client that starts `npp lsp`.

## Language features

The current language server provides diagnostics, keyword/builtin completion, context-aware struct-field and enum-variant completion, signature help, hover, go-to-definition, references, rename, document outline, workspace symbol search, whole-document/range formatting and quick fixes for common unknown-name errors. Workspace symbol search is capped at 500 results.

Navigation and type-aware completion are best-effort rather than a full compiler-backed index. Imported aliases and complete type inference for arbitrary member-expression chains are not implemented. A debugger/debug adapter and inlay hints are not included.

## Install N++ first

The extension does not bundle the compiler. Install N++ separately and make sure the `npp` command is available on PATH, or set `npp.serverPath` to the full path of the executable/launcher.

## Build a VSIX

Requirements: Node.js/npm and network access for the first dependency download.

```sh
npm install
npx --yes @vscode/vsce package
```

The current package version is **1.0.0**. The resulting VSIX includes its production language-client dependency; it does not bundle the compiler.

## Install

1. Open VS Code.
2. Press **Ctrl+Shift+P** (macOS: **Cmd+Shift+P**).
3. Run **Extensions: Install from VSIX...**.
4. Select `fanxcz.npp-language-1.0.0.vsix`.
5. Reload the window and open a `.npp` source file.

Or use:

```sh
code --install-extension fanxcz.npp-language-1.0.0.vsix
```

The extension is not yet published on the VS Code Marketplace; installing from VSIX is the available route.

## Commands and settings

- **N++: Show Language Server Output** opens the N++ Language Server output channel.
- `npp.serverPath` configures a custom compiler executable/launcher when `npp` is not on PATH.

Example settings JSON:

```json
{
  "npp.serverPath": "C:\\N++\\npp.cmd"
}
```

For startup failures, run `npp lsp` in a terminal and inspect the **N++ Language Server** output channel.
