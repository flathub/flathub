# Lutris Flatpak

## Build

```
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.gnome.Sdk//3.28
flatpak install --user flathub org.gnome.Platform//3.28
```

```
flatpak-builder --user --repo=lutris --force-clean build-dir org.lutris.Lutris.json
flatpak remote-add --user lutris lutris --no-gpg-verify
flatpak install --user lutris org.lutris.Lutris
flatpak run org.lutris.Lutris
```

## Development

The Python packages are built with https://github.com/flatpak/flatpak-builder-tools/tree/master/pip