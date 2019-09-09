# GZDoom Flatpak edition #

## Installation of gamedata ##
* Copy any commercial iwad into the folder `~/.config/gzdoom/`
* Optionally, configure the `~/.config/gzdoom/gzdoom.ini` file to load other directories

## Run with custom wads
Just as with the standalone GZDoom, you can pass commands through
```
flatpak run org.zdoom.GZDoom -file ~/.config/gzdoom/pwads/PL2.WAD

cd ~/.config/gzdoom/pwads/
flatpak run org.zdoom.GZDoom -file ./PL2.WAD
```

For more info, see
* https://zdoom.org/wiki/Command_line_parameters
* https://zdoom.org/wiki/Installation_and_execution_of_ZDoom

## Accessing files on unconventional spots ##
```
flatpak override org.zdoom.GZDoom --filesystem=/OTHER/LOCATION/WITH/WADS --user
```
 
