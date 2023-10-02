# Flatpak for furnace

the biggest multi-system chiptune tracker ever made. Please [visit the project website]() or [view the source code](https://github.com/tildearrow/furnace).

## Accessing bundled demos and instruments
The package includes demo music, instruments and sound samples at `/app/share/furnace` due to how the file system is sandboxed these can be difficult to access.

You can open their real location in the file manager by running:
```sh
flatpak run --command=gio org.tildearrow.furnace open /app/share/furnace
```
