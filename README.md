# org.nzbget.nzbget

Flatpak for [NZBGet](https://nzbget.com/).

## Install
```
flatpak install flathub org.nzbget.nzbget
```

## Run
```
flatpak run org.nzbget.nzbget
```

## Build from source

### Prerequisites:

For the current user `flatpak` is installed and the `flathub` repo is added.

### Build

```
flatpak install --user -y flathub org.flatpak.Builder
flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --repo=repo build-dir org.nzbget.nzbget.yml
```
or if your distribution has `flatpak-builder` installed:
```
flatpak-builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --repo=repo build-dir org.nzbget.nzbget.yml
```

### Run
```
flatpak run org.nzbget.nzbget
```

### Uninstall
```
flatpak uninstall org.nzbget.nzbget
```
