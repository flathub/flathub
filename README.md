# Q-Zandronum Flatpak edition

## Installation

### Game data
 
* Copy any commercial iwad into the folder:
  * `~/.var/app/com.qzandronum.Q-Zandronum/.config/zandronum/`
* Optionally, configure the ini to load files from a different directory:
  * `~/.var/app/com.qzandronum.Q-Zandronum/.config/zandronum/zandronum.ini` 

### Skins

* Copy any of your skins into folder:
  * `~/.var/app/com.qzandronum.Q-Zandronum/.config/zandronum/skins`


## Run with custom wads

### UI
With Doomseeker, you can create a custom game. Then, under mode you can sellect 'Play offline' to start a singleplayer game.

### CLI
Just as with the standalone Zandronum, you can pass commands through using the command line. If you want to play custom wads, you can add them to a sub-directory of `/zandronum/` and then you can directly access then from the terminal:

```
flatpak run --command="q-zandronum -file ~/.var/app/com.qzandronum.Q-Zandronum/.config/zandronum/pwads/PL2.WAD" com.qzandronum.Q-Zandronum
```

```
cd ~/.var/app/com.qzandronum.Q-Zandronum/.config/zandronum/pwads/
flatpak run com.qzandronum.Q-Zandronum -file ./PL2.WAD
```

For more info, see:

* https://wiki.zandronum.com/Command_Line_Parameters

Additionally:

* https://zdoom.org/wiki/Command_line_parameters
* https://zdoom.org/wiki/Installation_and_execution_of_ZDoom

## Accessing files on unconventional spots ##
If you want to access wads in different locations, you might have to adjust the [Flatpak sandboxing permissions](http://docs.flatpak.org/en/latest/sandbox-permissions.html). You can easily do that like this:

```
flatpak override com.qzandronum.Q-Zandronum --filesystem=/OTHER/LOCATION/WITH/WADS --user
```
 
# Credits
This repository was forked from [com.zandronum.Zandronum](https://github.com/flathub/com.zandronum.Zandronum), so most of the credit goes to there.
