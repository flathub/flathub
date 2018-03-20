# flatpak-openttd    

A flatpak package for the game openttd. Includes the opengfx, opensfx and openmsx resource packs, as well as timidity (needed to play music).     

## Building a release build     
First checkout the repo:     
```
git clone https://github.com/casept/flatpak-openttd     
cd flatpak-openttd   
git submodule init     
git submodule update     
```

Install the needed runtime and SDK:        
```
flatpak remote-add --user --if-not-exists --from gnome https://sdk.gnome.org/gnome.flatpakrepo       
flatpak install --user gnome org.freedesktop.Sdk 1.4      
flatpak install --user gnome org.freedesktop.Platform 1.4          
```

And build:         
```    
make     
```     
 
## Installation      
Just run `make install`. This will install the game for your local user.     

## Building/installing a trunk build
Follow the steps above, but use

```    
make -f Makefile-nightly     
make -f Makefile-nightly install
```     

instead of the standard `make` commands.

## Running        
In most desktop environments an openttd menu entry will have been created under games. If that's not the case you may need to log out and back in again. If you still can't find the icon your DE might not be looking in the directory where flatpak places .desktop files. If that's the case you'll have to run `flatpak run org.openttd.openttd` to start the game.

## Accessing save games/mods/etc.
For sandboxing reasons the game is configured to save user data to `~/.var/app/org.openttd.openttd/.openttd` instead of the usual `~/.openttd`. This directory will have the same structure as it does when the game is installed systemwide by normal means.
