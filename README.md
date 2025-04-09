# com.nzbget.nzbget

Flatpak for [NZBGet](https://nzbget.com/).

## Install
```
flatpak install flathub com.nzbget.nzbget
```

## Run
```
flatpak run com.nzbget.nzbget
```

## Build from source

### Prerequisites:

For the current user `flatpak` is installed and the `flathub` repo is added.

### Build

```
flatpak install --user -y flathub org.flatpak.Builder
flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --repo=repo build-dir com.nzbget.nzbget.yml
```
or if your distribution has `flatpak-builder` installed:
```
flatpak-builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --repo=repo build-dir com.nzbget.nzbget.yml
```

### Run
```
flatpak run com.nzbget.nzbget
```

### Uninstall
```
flatpak uninstall com.nzbget.nzbget
```
