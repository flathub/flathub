# Grayjay Desktop Flatpak (unofficial)

This is a flatpak for Grayjay Desktop. Please help test and report any issues you find.

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