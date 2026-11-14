# VS Code extension for N++

The repository contains the VS Code extension at `editors/vscode/`. It provides `.npp` syntax highlighting, snippets, and a client for the N++ Language Server.

## 1. Install N++

Install the N++ compiler first and confirm it runs in a terminal:

```sh
npp --version
npp doctor
```

On Windows, install the N++ portable bundle, add it to PATH and reopen VS Code. If you keep N++ elsewhere, use the `npp.serverPath` setting below.

## 2. Build the installable VSIX

Open a terminal at the root of your cloned `fanxcz/nplusplus` repository and run:

```sh
cd editors/vscode
npm install
npx --yes @vscode/vsce package
```

This creates `fanxcz.npp-language-1.0.0.vsix` in the extension directory. The filename is based on the extension publisher, package name, and version; it differs from the release artifact named `N++-Language-1.0.0.vsix` on the project downloads page. You need Node.js/npm and network access for this one-time dependency download. The resulting VSIX includes its runtime dependency; it does not bundle the N++ compiler.

## 3. Install in VS Code

1. Open VS Code.
2. Press **Ctrl+Shift+P** (on macOS, **Cmd+Shift+P**).
3. Run **Extensions: Install from VSIX...**.
4. Select `editors/vscode/fanxcz.npp-language-1.0.0.vsix`.
5. Reload VS Code when prompted.
6. Open any `.npp` source file. The language selector should display **N++**.

You can also install it from the repository root with:

```sh
code --install-extension editors/vscode/fanxcz.npp-language-1.0.0.vsix
```

The extension is not yet published on the VS Code Marketplace, so installing from VSIX is the available route.

## 4. Enable diagnostics and completion

The extension automatically starts `npp lsp` for `.npp` files. Run **N++: Show Language Server Output** from the command palette to inspect server startup and diagnostics. If your N++ launcher is not on PATH, open VS Code Settings (JSON) and add:

```json
{
  "npp.serverPath": "C:\\N++\\npp.cmd"
}
```

Use the **N++ Language Server** output channel to diagnose startup issues. The source tree currently implements diagnostics, completion, signature help, hover, definition/references/rename for indexed declarations, workspace symbols, formatting, and quick fixes. Resolution still has known type-inference and import-alias limits; a source-level debugger/debug adapter is not included.

## Troubleshooting

- **No colors:** reload VS Code and check the language selector says N++.
- **No completion or diagnostics:** run `npp lsp` in a terminal, verify PATH/`npp.serverPath`, then check the **N++ Language Server** output channel.
- **VSIX packaging fails:** run `npm install` in the extension directory, then retry `npx --yes @vscode/vsce package`.
