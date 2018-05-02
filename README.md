# GtkRadiant Flatpak build manifest
 
[GtkRadiant project website](https://icculus.org/gtkradiant/)
 
## Building
 Install flatpak-builder 0.10.10+ (svn support required) then run:

```
 flatpak-builder --force-clean --disable-updates --repo=gtkradiant-repo gtkradiant io.github.TTimo.GtkRadiant.json
 flatpak --user remote-add --no-gpg-verify --if-not-exists gtkradiant-repo gtkradiant-repo
```

## Running
If running flatpak 0.10.2+ with the wrapper bin path setup you can simply run the app as `io.github.TTimo.GtkRadiant`. Otherwise:
```
flatpak run io.github.TTimo.GtkRadiant [ COMMANDS ]
```

## FAQ
### BSP builds failing
If you have used another install of GtkRadiant it may have created some \*.proj files in _[MOD]/scripts_ that define different paths to the bsp build tools. Simply move or delete this folder (or the *.proj files inside). E.g:
```
mv ~/.q3a/baseq3/scripts ~/.q3a/baseq3/scripts.old
```

### Running bsp build and other commands directly
The other GtkRadiant tools can be run by passing the `--command` switch after `run`. All the bins are stored in `/app/gtkradiant`. Some examples:
```
flatpak run --command=/app/gtkradiant/q3map2 io.github.TTimo.GtkRadiant -info ~/.q3a/baseq3/maps/my-map.bsp
flatpak run --command=/app/gtkradiant/q3map2 io.github.TTimo.GtkRadiant -light -fast -threads 4 -fs_basepath /path/to/ioquake3 ~/.q3a/baseq3/maps/my-map.map
flatpak run --command=/app/gtkradiant/d3data io.github.TTimo.GtkRadiant model.qdt
```

The available binaries are: *q3data q3map2 q3map2_urt*.

## Flatpak build notes
So far I have concentrated on getting this working and have not done any clean-up on the output package. Thus it includes a whole bunch of binaries that are not needed as well as headers, pkg-configs and many other bits of cruft that bloat the package. But hey it works!
