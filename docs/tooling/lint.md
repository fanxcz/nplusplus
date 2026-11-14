# Linter

`npp lint` runs the current line-level checks and then parses and semantically analyzes one source file. With no file argument, it resolves the current project's configured entry source.

## Usage

```sh
# Lint the project's entry source
npp lint

# Check a specific source file
npp lint src/main.npp
```

## Current checks

| Code | Trigger | Behavior |
|---|---|---|
| `NX9001` | Trailing spaces or tabs at the end of a source line | Prints a warning with the file and line |
| `NX9002` | A line contains `TODO` or `FIXME` | Prints a warning with the file and line |

After these checks, N++ parses the program and runs its semantic analyzer. Syntax or semantic errors fail the command. Warnings are printed but do not currently fail the command by themselves.

## Limitations

The linter is an early implementation. It does not yet have configurable rules, automatic fixes, project-wide recursive file discovery, JSON/SARIF output, unused-variable analysis, dead-code analysis, or warning suppression attributes. The `--rule`, `--fix`, and `--format` options shown in older examples are not implemented.

Use `npp fmt --check src/main.npp` to verify the currently supported formatter behavior without modifying the source file. For the roadmap and planned static-analysis work, see [N++ Roadmap](../development/roadmap.md).
