# Doom64EX-Plus on Flatpak

This project contains files to build [Doom64EX-Plus](https://github.com/atsb/Doom64EX-Plus) as a Flatpak app.

## Building the app
Follow the [build guide](https://docs.flatpak.org/en/latest/building.html), basically you have to run:

```shell
$ flatpak-builder --user --verbose --install --install-deps-from=flathub --force-clean \
  build io.github.atsb.Doom64EX-Plus.yaml
```

## Copy game data files
If you already have required data files, just copy them in folder `~/.var/app/io.github.atsb.Doom64EX-Plus/data/doom64ex-plus`.
