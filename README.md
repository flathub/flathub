# GtkRadiant Flatpak build manifest

[GtkRadiant project website](https://icculus.org/gtkradiant/)

## Building
Use flatpak-builder 1.2.2 or newer (latest at the time of writing):

```
flatpak-builder --user --install gtkradiant io.github.TTimo.GtkRadiant.json
```

See flatpak-builder man page for more options. For repeat invocations, see `--force-clean` and `--disable-updates`.

## Running

```
flatpak run io.github.TTimo.GtkRadiant
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
