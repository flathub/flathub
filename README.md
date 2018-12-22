# vademecumshelf-flatpak

**Vade Mecum Shelf** is a collection of utilities wrapped into one single app, built with Electron.

![vademecumshelf-flatpak screenshot](vademecumshelf-flatpak.png)

[Homepage](https://github.com/tonton-pixel/vade-mecum-shelf)

This repo is about the flatpak package.

## Instructions

### Requirements

* [flatpak](https://github.com/flatpak/flatpak)
* [flatpak-builder](https://github.com/flatpak/flatpak-builder)

For EL7:

```
# yum install 'flatpak' 'flatpak-builder'
```

You may also wish to install the `xdg-desktop-portal*` packages:

```
# yum install 'xdg-desktop-portal*'
```

See also:

* [flatpak setup](https://flatpak.org/setup)

### Adding repository

```
$ flatpak remote-add --if-not-exists "flathub" "https://dl.flathub.org/repo/flathub.flatpakrepo"
```

See also:

* [flathub setup](http://docs.flatpak.org/en/latest/using-flatpak.html#add-a-remote)

### Prepare

```
$ flatpak install "flathub" "org.freedesktop.Sdk//18.08"
```

```
$ flatpak install "flathub" "org.freedesktop.Platform//18.08"
```

### Build

```
$ flatpak-builder "build" "com.github.tonton_pixel.VadeMecumShelf.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "com.github.tonton_pixel.VadeMecumShelf.yaml" "sh"
```

### Test run

```
$ flatpak-builder --run "build" "com.github.tonton_pixel.VadeMecumShelf.yaml" "VadeMecumShelf"
```

### Install

```
$ flatpak-builder --repo="repo" --force-clean "build" "com.github.tonton_pixel.VadeMecumShelf.yaml"
```

```
$ flatpak --user remote-add --no-gpg-verify "vademecumshelf" "repo"
```

```
$ flatpak --user install "vademecumshelf" "com.github.tonton_pixel.VadeMecumShelf"
```

### Run

```
$ flatpak run "com.github.tonton_pixel.VadeMecumShelf"
```

### Uninstall

```
$ flatpak --user uninstall "com.github.tonton_pixel.VadeMecumShelf"
```

```
$ flatpak --user remote-delete "vademecumshelf"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Are you the author of Vade Mecum Shelf?

No, I only created the flatpak package for it.

See also:

* [GitHub repo](https://github.com/tonton-pixel/vade-mecum-shelf)

