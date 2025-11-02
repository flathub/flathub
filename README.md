# FCast Receiver Flatpak

This is the official flatpak distribution for the FCast Receiver, a reference implementation of the receiving side of the FCast protocol wireless streaming of audio and video content between devices.

You can learn more about FCast at [fcast.org](https://fcast.org) or the README for the [github mirror](https://github.com/futo-org/fcast) of the source code.


## Building

You will need to have flatpak builder installed (`flatpak install org.flatpak.Builder`).

To build the flatpak:
- run `make build` to start a build
  - It may stop because you don't have some development flatpaks installed. If this happens, install them. Examples of these flatpaks may include `org.freedesktop.Sdk.Extension.node22` and `org.electronjs.Electron2.BaseApp` 

If you would like to run some additional validation steps, you can run commands like:
- `make lint`
- `make validate-meta`


## Preparing a new release

When a new release of the linux version of the receiver is released, these steps will need to be completed to ship a new version:

1. Update the commit hash of the fcast repository in the `org.fcast.Receiver.yaml` file to point to the commit hash of the new tagged version
2. ensure that `org.fcast.Receiver.metainfo.xml` has a new `<release>` added for this version
3. run `make prep-npm` to ensure that the `nom-sources.json` is up to date with all the necessary vendored NPM dependencies (flatpak builds do not have internet access once they start).
4. ensure the patches in the `patches` folder are up to date.
   - Notably this includes updating the `nonetwork.patch` file to include the filenames and hashes for the electron version used in the npm sources file. This prevents `electron-forge` from needing to check the hashes using the network (and therefore causing it to crash in flatpaks non-networked build environment. see [this issue](https://github.com/flatpak/flatpak-builder-tools/issues/452) for details)
5. Run the build locally (`make build`) and ensure it passes
6. Run the linter step (`make lint`)