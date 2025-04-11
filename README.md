
# Unison Flatpak build

Building this requires a patch to org.freedesktop.Sdk.Extension.ocaml to build
the "dune-configurator" library, which was just merged to the 24.08 branch.

The following features should work:
- local files
- SSH

Not supported:
- socket server (a bad idea anyway)
- fsmonitor (unknown if possible)

When using SSH, unison needs to be invoked on the remote end, which operates
independently of the unison started by the local user. By default, the
program "unison" will be invoked, which works if it is installed by
traditional packages, but not with Flatpak; to run the unison server via
Flatpak, put this in the profile file:

```
servercmd = flatpak run --command=unison io.github.bcpierce00.unison
```

The .desktop and .metainfo.xml files were taken from the Fedora package and
modified.

