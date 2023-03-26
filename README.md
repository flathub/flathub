# Space Station 14

This repo hosts an unofficial flatpak for
[Space Station 14](https://spacestation14.io/), the multiplayer disaster
simulator.

## Building

To build and install this project locally, use `flatpak-builder`:

    flatpak-builder build-dir com.spacestation14.Launcher.yaml --force-clean --repo=repo --install --user

## Creating a release

To update the version of SS14.Launcher, you will need to update the tag specified for the  `space-station-14-launcher` module in [com.spacestation14.Launcher.yaml](./com.spacestation14.Launcher.yaml), and then update the nuget sources which are generated using `flatpak-dotnet-generator`. To do that automatically, use the [update-ss14-sources.py](./tools/update-ss14-sources.py) tool:

    ./tools/update-ss14-sources.py

Finally, remember to add information about the new release to `modules/data/com.spacestation14.Launcher.appdata.xml`.

## Authors

- Dylan McCall <dylan@dylanmccall.ca>
