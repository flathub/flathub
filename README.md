# Headlamp (Flatpak build files)

[Headlamp](https://github.com/kinvolk/headlamp/) is an easy-to-use and
extensible Kubernetes web UI.

Headlamp was created to be a Kubernetes web UI that has the traditional functionality of other web UIs/dashboards available (i.e. to list and view resources) as well as other features.

This repository has the necessary files to build Headlamp as a Flatpak.

## Dependencies

Flatpak builds apps by previously fetching and caching its dependencies so the build
happens without access to the network.

Headlamp's dependencies are declared in the following files:
- go-generated-sources.json
- node-generated-sources-app.json
- node-generated-sources-frontend.json

## Updating to a new Headlamp version

When a new version of Headlamp is out, its tag should be set as the headlamp's
module git branch in the manifest, and the generated sources should be updated for
that version.

### Updating generated sources

The generated sources are not supposed to be edited manually.
Instead, the [update-sources.sh](./update-sources.sh) script should be used.
This script uses the version set up in the [Flatpak manifest](./io.kinvolk.Headlamp.yaml)
to generate the sources for it.

To print/double-check the version that the `update-sources.sh` script uses, run:

```bash
./update-sources.sh -v
```

To update the generated sources for the currently set up version, run:

```bash
./update-sources.sh
```

**Important:** The `update-sources.sh` script uses the
[flatpak-builder-tools](https://github.com/flatpak/flatpak-builder-tools/) scripts
but needs the PRs [195](https://github.com/flatpak/flatpak-builder-tools/pull/195)
and [199](https://github.com/flatpak/flatpak-builder-tools/pull/199).

You can fetch those PRs' commits to a local clone of the `flatpak-builder-tools` and
and make the `update-sources.sh` use it by doing:
```bash
FLATPAK_BUILDER_TOOLS=/path/to/flatpak-builder-tools ./update-sources.sh
```

One the PRs are merged, this section will be updated and the `flatpak-builder-tools`
will be added as a submodule.

## License

The code in this repository is licensed as [Apache 2.0](./LICENSE).
