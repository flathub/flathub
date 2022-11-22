# Minion 3
Flatpak wrapper for Minion, a premier addon manager.

## Information
This version of Minion is using Java. As it requires JavaFX and it's not provided by OpenJDK it was settled to use [Bellsoft Liberica JRE 11](https://bell-sw.com/pages/downloads/) as it provided all the needed libraries. 

It is also compatible with a flatpak installation of ESO through flatpak Steam.

## Disclaimer
Because of how flatpak and minion work, the app can not be updated automatically by itself. Meaning all of the micro-changes in-between will not show up
unless they post a new version of it and it's updated here.

For some reason it is not possible to run this flatpak from a toolbox container on fedora. It works when you install it in a container and then just use the host to start it.

## Building and runnning

Building:

    flatpak-builder --user --install --force-clean build-dir com.mmoui.Minion.yml

Bulding in a silverblue toolbox container:

    flatpak-builder --user --install --force-clean build-dir com.mmoui.Minion.yml --disable-rofiles-fuse

Checking Minion version for updates:

    flatpak run org.flathub.flatpak-external-data-checker com.mmoui.Minion.yml

Running:

    flatpak run com.mmoui.Minion

## Screenshots

### Showing installed addons
![installed](https://raw.githubusercontent.com/zastrixarundell/flathub/com.mmoui.Minion/master/screenshots/installed.png)

### Showing available addons for download
![find](https://raw.githubusercontent.com/zastrixarundell/flathub/com.mmoui.Minion/master/screenshots/find.png)


### Searching for addons
![search](https://raw.githubusercontent.com/zastrixarundell/flathub/com.mmoui.Minion/master/screenshots/search.png)


*This is not affiliated with Minion, MMOUI nor Good Game Mods, LLC, it is just simply a wrapper for their client.*