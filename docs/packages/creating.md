# Creating Packages

N++'s current package workflow is local-first: a package is a source directory with a `npp.toml` file which another project can vendor with `npp pkg add`.

## Minimum structure

```text
my-library/
├── npp.toml
├── src/
│   └── main.npp
├── README.md
└── LICENSE
```

## Manifest

Start with a package created from a N++ template, or add a manifest like:

```toml
[package]
name = "my-library"
version = "1.0.0"

[build]
entry = "src/main.npp"
artifact = "my-library"
target = "native"
```

Package names must contain only letters, digits, underscores or hyphens. Ensure the configured entry file exists and passes `npp check`.

## Local development workflow

```sh
# Create a package skeleton
npp new my-library --template library
cd my-library
npp check
npp build

# In a separate application project:
npp pkg add ../my-library
npp pkg list
npp pkg remove my-library
```

`npp pkg add` copies files into the consuming project's `vendor/`, skipping Git/editor state, build output, distribution output and node modules. It writes a dependency entry into `npp.toml` and refreshes the lockfile. Removing a package deletes its vendored source and clears the matching manifest/lockfile entries.

## Create a local archive

```sh
npp pkg publish
```

This writes a ZIP-formatted `.nxpkg` archive into `dist/`. It does **not** publish to an online registry; registry authentication, uploads, version constraints, dependency solving and transitive builds are not implemented.

See [Using Packages](using.md) and the [N++ roadmap](../development/roadmap.md) for future package-manager goals.
