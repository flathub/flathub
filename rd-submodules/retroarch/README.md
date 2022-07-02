# RetroArch on Flathub

[Flathub](https://flathub.org/) is the central place for building and hosting [Flatpak](http://flatpak.org/) builds. Go to https://flathub.org/builds/ to see Flathub in action.

[RetroArch](http://retroarch.com) is a frontend for emulators, game engines and media players.

## Installation

To install RetroArch through Flathub, use the following:
```
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user -y flathub org.libretro.RetroArch
```

## Usage

1. Run RetroArch through Flatpak:
    ```
    flatpak run org.libretro.RetroArch
    ```

2. [Install some libretro cores](https://docs.libretro.com/guides/download-cores/) using the Online Updater. There is no need to update core info files, assets, joypad profiles, cheats, database, cg, glsl, or slang shaders, as those are shipped with the Flatpak.

3. [Import content](https://docs.libretro.com/guides/import-content/) by scanning the folder where your games are kept.

4. [Launch content](https://docs.libretro.com/guides/launch-content/) through RetroArch either through the menu, or through the command line:
    ```
    flatpak run org.libretro.RetroArch -L ~/.var/app/org.libretro.RetroArch/config/retroarch/cores/chailove_libretro.so FloppyBird.chailove
    ```

## Known Issues

There are a few known issues with using the Flatpak build of RetroArch, due to its sandboxed nature...

### Joypad Driver

Upstream udev support for Flatpak isn't quite there yet. Because of this, the Flatpak build of RetroArch uses the SDL driver, which lacks some of the controller autoconfigs. If you have a controller that isn't detected, feel free to submit the SDL config for it over at [retroarch-joypad-autoconfig](https://github.com/libretro/retroarch-joypad-autoconfig).

### Core Libraries

There are a few libretro cores that have been been compiled with dependencies on libraries that are not currently deployed with Flatpak. If you find one, you are invited to [create an issue](https://github.com/flathub/org.libretro.RetroArch/issues) detailing what's missing so that we can ship it in the sandboxed Flatpak.

## Update

To update RetroArch through Flathub, use the follow command:
```
flatpak update --user org.libretro.RetroArch
```

### Options

Through the [Flatpak command line arguments](http://flatpak.org/flatpak/flatpak-docs.html), it is possible to change how RetroArch is used.

#### Mounted Directories

Allow Flatpak access to different mounted drives through using the `--filesystem` option:
```
flatpak run --filesystem=host --filesystem=/media/NAS/roms org.libretro.RetroArch
```

## Development

To test the application locally, use [flatpak-builder](http://docs.flatpak.org/en/latest/flatpak-builder.html) with:

```
git clone https://github.com/flathub/org.libretro.RetroArch.git
cd org.libretro.RetroArch
git submodule update --init
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak-builder builddir --install-deps-from=flathub --user --install --force-clean org.libretro.RetroArch.json
flatpak run org.libretro.RetroArch --verbose
```

### Update

To push up a newer version of RetroArch to Flathub, take on the following:

1. Edit [org.libretro.RetroArch.json](https://github.com/flathub/org.libretro.RetroArch/blob/master/org.libretro.RetroArch.json)
2. Change all commit hashes to the latest tag, and commit hashes for each repository
3. Edit [org.libretro.RetroArch.appdata.xml](https://github.com/flathub/org.libretro.RetroArch/blob/master/org.libretro.RetroArch.appdata.xml)
4. Create a new `<release>` element and list the latest [RetroArch CHANGES.md](https://github.com/libretro/RetroArch/blob/master/CHANGES.md) that apply to the Linux build
5. Push up a Pull Request with the new changes
6. Wait to see if Flathub bot approves the changes
7. Merge the Pull Request and tag the release

### Clean

```
flatpak uninstall --user org.libretro.RetroArch
rm -rf ~/.var/app/org.libretro.RetroArch .flatpak-builder
```
