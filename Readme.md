# Hoptodesk Flatpak repo

## Prerequsites
- flatpak
- flatpak-builder

## Installing dependecnies
### Debian/Ubuntu
```bash
apt-get update && apt-get -y install flatpak flatpak-builder
```

## Adding Flathub
```bash
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
```

## Building Flatpak
```bash
flatpak-builder --force-clean --user --install-deps-from=flathub --repo=repo --install builddir com.hoptodesk.HopToDesk.json
```

## Exporting Flatpak
```bash
flatpak build-bundle repo hoptodesk.flatpak com.hoptodesk.HopToDesk --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo
```
