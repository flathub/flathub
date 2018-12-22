# unicodeplus-flatpak

**Unicode Plus** is a set of Unicode, Unihan and emoji utilities wrapped into one single app, built with Electron.

![unicodeplus-flatpak screenshot](unicodeplus-flatpak.png)

[Homepage](https://github.com/tonton-pixel/unicode-plus)

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
$ flatpak-builder "build" "com.github.tonton_pixel.UnicodePlus.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "com.github.tonton_pixel.UnicodePlus.yaml" "sh"
```

### Test run

```
$ flatpak-builder --run "build" "com.github.tonton_pixel.UnicodePlus.yaml" "UnicodePlus"
```

### Install

```
$ flatpak-builder --repo="repo" --force-clean "build" "com.github.tonton_pixel.UnicodePlus.yaml"
```

```
$ flatpak --user remote-add --no-gpg-verify "unicodeplus" "repo"
```

```
$ flatpak --user install "unicodeplus" "com.github.tonton_pixel.UnicodePlus"
```

### Run

```
$ flatpak run "com.github.tonton_pixel.UnicodePlus"
```

### Uninstall

```
$ flatpak --user uninstall "com.github.tonton_pixel.UnicodePlus"
```

```
$ flatpak --user remote-delete "unicodeplus"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Are you the author of Unicode Plus?

No, I only created the flatpak package for it.

See also:

* [GitHub repo](https://github.com/tonton-pixel/unicode-plus)

