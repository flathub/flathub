# QC:Doom Edition Flatpak release

A gameplay mod bringing Quake Champions weapons and many different player classes (Champions) each with unique abilities, passive and actives, to be used in single player or multiplayer; along with monsters from Quake 1, Quake 2 and Quake 4, and Doom 2016 for you to chose what theme of monsters you want, or even mix them up.  
The mod was designed from the ground up to be played in single player, cooperative, and many types of PvP, like Deathmatch (Free for all), Team Deathmach, Duel, Capture the Flag, Last Man Standing, "Clan Arena".  
See the [QCDE Manual](https://qcde.net/files/public/QCDE_Manual.pdf) for more information.

## Installation

This Flatpak release of QC:DE combines Q-Zandronum, Doomseeker and the mod itself into a complete, pre-configured package for easy installation on Linux systems.

### Game data

* Pre-installed game data can be found under:
  * `/app/share/games/doom` where the location of `/app/` can be queried with the following command: `flatpak info --show-location net.qcde.QCDE`

* The default download location of Doomseeker is:
  * `~/.var/app/net.qcde.QCDE/data/doomseeker`

* Copy any commercial iwad into the folder:
  * `~/.var/app/net.qcde.QCDE/.config/zandronum/`
* Optionally, edit the Q-Zandronum settings file to load files additional directories:
  * `~/.var/app/net.qcde.QCDE/.config/zandronum/zandronum.ini`
  * Add new paths under the `[IWADSearch.Directories]` and `[FileSearch.Directories]` sections, don't edit the default ones.
  * Make sure to also add any new search paths to Doomseeker as well under `Settings -> File Paths`, otherwise the server browser will not see them.

## Run with custom wads

### UI
With Doomseeker, you can create a custom game. Then, under mode you can sellect 'Play offline' to start a singleplayer game.  
An example Deathmatch game is pre-configured out of the box.

### CLI
Just as with the standalone Zandronum, you can pass commands through using the command line. If you want to play custom wads, you can add them to a sub-directory of `/zandronum/` and then you can directly access then from the terminal:

```
flatpak run --command="q-zandronum -file ~/.var/app/net.qcde.QCDE/.config/zandronum/pwads/PL2.WAD" net.qcde.QCDE
```

```
cd ~/.var/app/net.qcde.QCDE/.config/zandronum/pwads/
flatpak run net.qcde.QCDE -file ./PL2.WAD
```

For more info, see:

* https://wiki.zandronum.com/Command_Line_Parameters

Additionally:

* https://zdoom.org/wiki/Command_line_parameters
* https://zdoom.org/wiki/Installation_and_execution_of_ZDoom

## Accessing files on unconventional spots ##
If you want to access wads in different locations, you might have to adjust the [Flatpak sandboxing permissions](http://docs.flatpak.org/en/latest/sandbox-permissions.html). You can easily do that like this:

```
flatpak override net.qcde.QCDE --filesystem=/OTHER/LOCATION/WITH/WADS --user
```
 
# Credits
This repository is based on the Flatpak release of Q-Zandronum [com.qzandronum.Q-Zandronum](https://github.com/flathub/com.qzandronum.Q-Zandronum), which in turn, was forked from [com.zandronum.Zandronum](https://github.com/flathub/com.zandronum.Zandronum).  
Credits go to every contributor of these repositories.

