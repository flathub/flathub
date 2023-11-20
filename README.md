# Floorp for Flatpak (one.ablaze.floorp)

Flathub: https://flathub.org/apps/one.ablaze.floorp

Browser source code: https://github.com/Floorp-Projects/Floorp

### Based on

https://github.com/flathub/org.mozilla.firefox.BaseApp

https://searchfox.org/mozilla-central/source/taskcluster/docker/firefox-flatpak

## How to install (from Flathub)
```
flatpak install flathub one.ablaze.floorp
```

## How to build
```sh
flatpak-builder build-dir one.ablaze.floorp.yml
```
To test:
```sh
flatpak-builder --run build-dir one.ablaze.floorp.yml floorp
```
