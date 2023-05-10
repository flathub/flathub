# Minion 3
Flatpak wrapper for Minion, a premier addon manager.

## Disclaimer (important to read)

### Regarding Hardware Acceleration

As hardware acceleration doesn't work well with XWayland under Wayland with the closed-source Nvidia drivers, hardware acceleration was disabled to have higher coverage for GPUs.

One may enable hardware acceleration by running `flatpak override --user --device=dri gg.minion.Minion`. Be wary, this will cause flickering for Minion if you're using the proprietary Nvidia driver while running Wayland.

### First start

After running minion for the first time while there are mods in the folder, minion will seem frozen. After a restart Minion will start working again and the issue shouldn't occur anymore.

## Addon snapshots

Minion doesn't support backups of AddOns under Linux as the expected file system path differs than on wdinows. This should be addressed in Minion 4 when it comes out.

## Information about the project
This version of Minion is using Java. As it requires JavaFX and it's not provided by OpenJDK it was settled to use [Bellsoft Liberica JRE 11](https://bell-sw.com/pages/downloads/) as it provided all the needed libraries. 

It is also compatible with a flatpak installation of ESO through flatpak Steam and hopefully Lutris.

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