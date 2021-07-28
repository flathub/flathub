Work-in-progress Flatpak manifest for Gobby.

The app ID is chosen to match the id in Gobby's appstream. The underscore is because components of a Flatpak app ID may not begin with a digit.

Build and install with:

```
flatpak install flathub org.gnome.Platform//40 org.gnome.Sdk//40
flatpak-builder --force-clean --user --install app de._0x539.gobby.json
```
