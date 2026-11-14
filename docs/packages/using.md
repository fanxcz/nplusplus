# Using Packages

N++ currently supports **local vendored packages**. Git dependencies, version-range resolution and a hosted registry are not implemented. The local package commands update `npp.toml`, copy source into `vendor/`, and refresh `npp.lock`.

## Add a local package

The source directory must contain `npp.toml` and use a package name made from letters, digits, underscores or hyphens.

```sh
npp pkg add ../my-library
npp pkg list
```

The source is copied to `vendor/<package-name>/`. Transient `.git`, `.vscode`, `build`, `dist`, `node_modules` and `CMakeFiles` directories and symlinks are not copied. Adding the current project as its own dependency, or choosing a source that contains the destination vendor path, is rejected.

## Remove a package

```sh
npp pkg remove my-library
```

The command removes `vendor/my-library`, deletes that package's entry from the `[dependencies]` section in `npp.toml`, and regenerates `npp.lock`. Path separators and traversal names are rejected; unrelated dependencies are preserved.

## Lockfile behavior

The lockfile records the project name/version and dependency versions. For vendored local packages, N++ reads the version from the package's own manifest:

```toml
# N++ lockfile v1
package = "my-app"
version = "1.0.0"

[dependencies]
my-library = "0.3.0"
```

Regenerate the lockfile from `npp.toml` with:

```sh
npp pkg update
```

## Current limitations

This is not a full package solver yet. There are no remote registries, Git fetching, semver range resolution, transitive dependency solver, checksums, or guaranteed reproducible source verification. `npp pkg publish` creates a local ZIP-formatted `.nxpkg` archive under `dist/`; it does not upload anything to a server. `npp pkg build` builds the configured entry source, not a dependency graph.

See [Creating Packages](creating.md) and the [development roadmap](../development/roadmap.md).


## Verify vendored package contents

Before building or after reviewing dependency changes, run:

```sh
npp pkg verify
```

This compares each recorded local package with SHA-256 hashes in `npp.lock`. For an intentional change, review the diff and run `npp pkg update` to refresh the checksum. This detects local content modifications; it is not a publisher signature and cannot establish that a package is trustworthy.
