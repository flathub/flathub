# Grayjay Desktop Flatpak (unofficial)

This is a flatpak for Grayjay Desktop. Please help test and report any issues you find.

## Running the flatpak:

If you just want to install this flatpak to help test it, follow these instructions.

*Note*: These instructions assume your flatpaks are stored in your home directory (i.e. a user install)

1. go grab the bundle file from the latest release on this github repo
2. run something like `flatpak build-import-bundle  ~/.local/share/flatpak/repo <bundle file>` to import it
3. run `patch.sh` from this repository (or manually review it and copy the files from the grayjay installer zip yourself). This is a temporary workaround that will be resolved prior to uploading to flathub.
   - you can run `unpatch.sh` to remove the files


Give Grayjay a try and see if you can find any issues that don't already exist in the issues tab.

## Building Locally

1. install `flatpak-builder` (required) and `just` (optional)
2. run `just build` or `flatpak-builder --user --install build-dir app.grayjay.Desktop.yaml` to build the flatpak
   1. there is also the `clean-build` shortcut which adds the `--force-clean` arg
3. run `just run` or `flatpak run app.grayjay.Desktop` or open it from your system menu to run the flatpak.

## Documentation

Here is some documentation on building flatpaks that has been helpful

- https://docs.flathub.org/docs/for-app-authors/requirements/
- https://docs.flathub.org/docs/for-app-authors/submission/#before-submission
- https://docs.flathub.org/docs/for-app-authors/requirements/#dependency-manifest
- https://docs.flathub.org/docs/for-app-authors/metainfo-guidelines/
- https://flatpak-docs.readthedocs.io/en/latest/first-build.html
- https://docs.flatpak.org/en/latest/sandbox-permissions.html