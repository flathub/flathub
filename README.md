# Grayjay Desktop Flatpak (unofficial)

This is an attempt to create a flatpak for Grayjay Desktop

## Testing

1. install `flatpak-builder`
2. `flatpak-builder --user --install --force-clean build-dir app.grayjay.Desktop.yaml`
3. `flatpak run app.grayjay.Desktop` (this is currently the part that doesn't work)

## Documentation

Here is some documentation thats been helpful to me so far:

**Basics/stuff I've already used**
- https://docs.flathub.org/docs/for-app-authors/requirements/
- https://docs.flathub.org/docs/for-app-authors/submission/#before-submission
- https://docs.flathub.org/docs/for-app-authors/requirements/#dependency-manifest
- https://docs.flathub.org/docs/for-app-authors/metainfo-guidelines/
- https://flatpak-docs.readthedocs.io/en/latest/first-build.html

**Currently useful Stuff**
- https://docs.flatpak.org/en/latest/sandbox-permissions.html
- 