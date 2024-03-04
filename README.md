# Raze Flatpak edition

Raze is a fork of Build engine games backed by GZDoom tech and combines Duke Nukem 3D, Blood, Redneck Rampage, Shadow Warrior and Exhumed/Powerslave in a single package. It is also capable of playing Nam and WW2 GI.

## Installation of game-data

* Copy any commercial iwad into the folder `~/.var/app/org.zdoom.Raze/.config/raze/`
* Optionally, configure the `~/.var/app/org.zdoom.Raze/.config/raze/raze.ini` file to load other directories

## Run with mods
Just as with the stand-alone Raze, you can pass commands through using the command line. If you want to play custom wads, you can add them to a sub-directory of `/raze/` and then you can directly access then from the terminal:

```
flatpak run org.zdoom.Raze -file ~/.var/app/org.zdoom.Raze/.config/raze/pmods/PL2.WAD
```

```
cd ~/.var/app/org.zdoom.Raze/.config/raze/pmods/
flatpak run org.zdoom.Raze -file ./pmods/PL2.WAD
```

For more info, see:

* https://zdoom.org/wiki/Command_line_parameters
* https://zdoom.org/wiki/Installation_and_execution_of_ZDoom

## Accessing files
If you want to access wads in different locations, you might have to adjust the [Flatpak sandboxing permissions](http://docs.flatpak.org/en/latest/sandbox-permissions.html). 

### GUI
You can use Flatseal to visually manage your permissions:
https://flathub.org/apps/details/com.github.tchx84.Flatseal

### Terminal

You can easily do that like this:

```
flatpak override org.zdoom.Raze --filesystem=/OTHER/LOCATION/WITH/GRPS --user
```
 
