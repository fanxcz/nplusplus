# Publishing Packages

## Current behavior

N++ does **not** have a hosted package registry. The implemented `npp pkg publish` command creates a ZIP-formatted `.nxpkg` archive in the project's `dist/` directory; it does not upload it anywhere.

## Create a local archive

Run this from a directory containing a valid `npp.toml` manifest:

```sh
npp check
npp fmt --check
npp lint
npp pkg publish
```

The formatter and linter reflect their current limited implementations; review their output and run the project's actual tests before distributing an archive. The archive is a local file that you can share separately.

## Add a package from source

A consumer project can vendor a local **source directory** with:

```sh
npp pkg add ../my-library
npp pkg list
```

The `pkg add` command expects a package directory, not the generated `.nxpkg` archive. See [Using Packages](using.md) for the supported local workflow.

## Not available yet

The CLI does not implement registry login, registry upload, remote package lookup, publish dry-runs, unpublishing/yanking, package ownership, private registries, or version-range solving. Commands such as `npp pkg login`, `npp pkg info`, and `npp pkg unpublish` should not be used; they are not implemented.
