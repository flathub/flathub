# Space Station 14

This repo hosts an unofficial flatpak for
[Space Station 14](https://playss14.com), the multiplayer disaster
simulator.

## Building

To build and install this project locally, use `flatpak-builder`:

    flatpak-builder build-dir com.playss14.launcher.yaml --force-clean --repo=repo --install --user

## Creating a release

To update the version of SS14.Launcher, you will need to update the tag specified for the  `space-station-14-launcher` module in [com.playss14.launcher.yaml](./com.playss14.launcher.yaml), and then update the nuget sources which are generated using `flatpak-dotnet-generator`. To do that automatically, use the [update-ss14-sources.py](./tools/update-ss14-sources.py) tool:

    ./tools/update-ss14-sources.py

Finally, remember to add information about the new release to `modules/data/com.playss14.launcher.appdata.xml`.

## Authors

- Oleksandr Volkogon <alexivolkov@vivaldi.net>
