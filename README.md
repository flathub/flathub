# com.google.EarthPro
Flatpak for Google Earth Pro

## Prerequisite

- `flatpak`, `flatpak-builder` packages
- Runtime `org.freedesktop.Platform` version `22.08`
- Runtime `org.freedesktop.Sdk` version `22.08`

## Method 1:

### Build and install Google Earth Pro
```bash
flatpak-builder --user --install --force-clean  flatpakbuildir com.google.EarthPro.yaml
```
### Run Google Earth Pro
```bash
flatpak run com.google.EarthPro
```
### Uninstall Google Earth Pro
```bash
flatpak uninstall --user com.google.EarthPro
```


## Method 2:

### Build Google Earth Pro
```bash
flatpak-builder --repo=repo --force-clean flatpakbuildir com.google.EarthPro.yaml
```
### Add Google Earth Pro local repo
```bash
flatpak remote-add --user myGoogle Earth Pro repo
```
### Install Google Earth Pro from local repo
```bash
flatpak install --user myGoogle Earth Pro com.google.EarthPro
```
### Run Google Earth Pro
```bash
flatpak run com.google.EarthPro
```
### Uninstall Google Earth Pro
```bash
flatpak uninstall --user com.google.EarthPro
```
