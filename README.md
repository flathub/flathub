# com.getgrist.grist

Flatpak for [Grist Desktop](https://www.getgrist.com/), spreadsheet software to end data chaos.

## Build

To build and install the Flatpak locally, run

```sh
flatpak-builder build com.getgrist.grist.yml --force-clean --install --user
```

## Updating to a new Grist version

1. Find out the latest grist-desktop release from its [releases](https://github.com/gristlabs/grist-desktop/releases).
   Make note of the _tag_ name (which starts with `v`).

2. Find out the corresponding grist-core release, it will usually be mentioned in the grist-desktop release.
   Otherwise, browser the source of the grist-desktop release tag, find the `core` commit, and find the
   version in the [grist-core repository](https://github.com/gristlabs/grist-core).
   Make note of the _tag_ name (which starts with `v`).

3. Run the update script with grist-desktop and grist-core release.
   For example, if you have grist-desktop `v0.3.10` the grist-core tag is `v1.7.11`, then run

   ```sh
   ./update.sh v0.3.10 v1.7.11
   ```

4. Build the flatpak with `flatpak-builder build com.getgrist.grist.yml --install --user`,
   and run it with `flatpak run com.getgrist.grist`. Make sure it works well.

Note that pyodide is not updated by the update script, you currently need to take care of this yourself
if the used version changes.


## Updating pyodide

This currently requires a manual step.

1. Update `PYODIDE_VERSION` in [`update.sh`](update.sh), and run the update script.

2. In [the manifest `com.getgrist.grist.yml`](com.getgrist.grist.yml), update version in the `pyodide xbuildenv env` command.

3. Find the version in https://pyodide.github.io/pyodide/api/pyodide-cross-build-environments.json

  a. Check the Python version matches what is in the base image. If not, either don't upgrade, or install and use that Python version to build.

  b. Update the `xbuildenv` URL and hash in the manifest module's sources.

  c. Check the emscripten version, and update `emsdk` version in the manifest module's sources.

4. Figure out what URLs `emsdk`'s components use and integrate it into the manifest module's sources (needs to be worked out more).
   One way is trying to build, figuring out what URLs cannot be fetched, and adding them manually.
   Emscripten has config files for [tools](https://github.com/emscripten-core/emsdk/blob/4.0.9/emsdk_manifest.json)
   and [release tags](https://github.com/emscripten-core/emsdk/blob/4.0.9/emscripten-releases-tags.json) (remember to pick relevant emscripten tag),
   which may help figuring out URLs.

