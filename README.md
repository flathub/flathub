# com.biglybt.BiglyBT
Flatpak for BiglyBT (https://www.biglybt.com/)

## Prerequisite

- `flatpak`, `flatpak-builder` packages
- Runtime `org.gnome.Platform` version `42`
- Runtime `org.gnome.Sdk` version `42`
- Runtime Extension `org.freedesktop.Sdk.Extension.openjdk11`

## Method 1:

### Build and install BiglyBT
```
flatpak-builder --user --install --force-clean  flatpakbuildir com.biglybt.BiglyBT.yaml
```
### Run BiglyBT
```
flatpak run com.biglybt.BiglyBT
```
### Uninstall BiglyBT
```
flatpak uninstall --user com.biglybt.BiglyBT
```


## Method 2:

### Build BiglyBT
```
flatpak-builder --repo=repo --force-clean flatpakbuildir com.biglybt.BiglyBT.yaml
```
### Add BiglyBT local repo
```
flatpak remote-add --user mybiglybt repo
```
### Install BiglyBT from local repo
```
flatpak install --user mybiglybt com.biglybt.BiglyBT
```
### Run BiglyBT
```
flatpak run com.biglybt.BiglyBT
```
### Uninstall BiglyBT
```
flatpak uninstall --user com.biglybt.BiglyBT
```
