# Minion 3
Flatpak wrapper for Minion, a premier addon manager.

## Information
This version of Minion is using Java. As it requires JavaFX and it's not provided by OpenJDK it was settled to use [Bellsoft Liberica JRE 11](https://bell-sw.com/pages/downloads/) as it provided all the needed libraries. 

It is also compatible with a flatpak installation of ESO through flatpak Steam.

## Building and runnning

Building:

    flatpak-builder --user --install --force-clean build-dir gg.minion.Minion.yml

Bulding in a silverblue toolbox container:

    flatpak-builder --user --install --force-clean build-dir gg.minion.Minion.yml --disable-rofiles-fuse

Checking Minion version for updates:

    flatpak run org.flathub.flatpak-external-data-checker gg.minion.Minion.yml

Running:

    flatpak run gg.minion.Minion

## Screenshots

### Showing installed addons
![installed](https://raw.githubusercontent.com/zastrixarundell/flathub/gg.minion.Minion/screenshots/installed.png)

### Showing available addons for download
![find](https://raw.githubusercontent.com/zastrixarundell/flathub/gg.minion.Minion/screenshots/find.png)


### Searching for addons
![search](https://raw.githubusercontent.com/zastrixarundell/flathub/gg.minion.Minion/screenshots/search.png)