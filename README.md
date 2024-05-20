# FireDragon for Flatpak (org.garudalinux.firedragon)

Flathub: https://flathub.org/apps/org.garudalinux.firedragon

Browser source code: https://gitlab.com/garuda-linux/firedragon/builder

### Based on

https://github.com/flathub/org.mozilla.firefox.BaseApp

https://searchfox.org/mozilla-central/source/taskcluster/docker/firefox-flatpak

## How to install (from Flathub)

``` sh
flatpak install flathub org.garudalinux.firedragon
```

## How to build

``` sh
flatpak-builder build org.garudalinux.firedragon.yml
```

To test:

``` sh
flatpak-builder --run build org.garudalinux.firedragon.yml firedragon
```
