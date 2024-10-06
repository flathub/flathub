# Node22 Flatpak

## Building

```
flatpak-builder build-dir --user --ccache --force-clean --install org.freedesktop.Sdk.Extension.node22.yaml
```

Then you can run it via the command line:

```
flatpak run org.freedesktop.Sdk.Extension.node22
```

or just search for the installed app on your system
