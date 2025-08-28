# FireDragon (Catppuccin Edition) for Flatpak (org.garudalinux.firedragon-catppuccin)

Flathub: https://flathub.org/apps/org.garudalinux.firedragon-catppuccin

Browser source code: https://gitlab.com/garuda-linux/firedragon/firedragon12

### Based on

https://github.com/flathub/org.mozilla.firefox.BaseApp

https://searchfox.org/mozilla-central/source/taskcluster/docker/firefox-flatpak

## How to install (from Flathub)

``` sh
flatpak install flathub org.garudalinux.firedragon-catppuccin
```

## How to build

``` sh
flatpak-builder build org.garudalinux.firedragon-catppuccin.yml
```

To test:

``` sh
flatpak-builder --run build org.garudalinux.firedragon-catppuccin.yml firedragon
```
