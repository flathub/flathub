# xnsketch-flatpak

**XnSketch** allows you to turn your photos into cartoon or sketch images.

![xnsketch-flatpak screenshot](xnsketch-flatpak.png)

[Homepage](https://www.xnview.com/en/xnsketch/)

This repo is about flatpak package.

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

```
$ git submodule init
```

```
$ git submodule update
```

### Build

```
$ flatpak-builder "build" "com.xnview.XnSketch.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "com.xnview.XnSketch.yaml" "sh"
```

### Test run

```
$ flatpak-builder --run "build" "com.xnview.XnSketch.yaml" "xnsketch"
```

### Install

```
$ flatpak-builder --repo="repo" --force-clean "build" "com.xnview.XnSketch.yaml"
```

```
$ flatpak --user remote-add --no-gpg-verify "xnsketch" "repo"
```

```
$ flatpak --user install "xnsketch" "com.xnview.XnSketch"
```

### Run

```
$ flatpak run "com.xnview.XnSketch"
```

### Uninstall

```
$ flatpak --user uninstall "com.xnview.XnSketch"
```

```
$ flatpak --user remote-delete "xnsketch"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Does flatpak-ed XnSketch run as superuser?

[No](https://github.com/flatpak/flatpak/issues/1557). It is a [MATE](https://github.com/mate-desktop)/[marco](https://github.com/mate-desktop/marco) [issue](https://github.com/mate-desktop/marco/issues/301).

### Is this freeware?

[Yes](https://www.xnview.com/en/xnsketch/#downloads).

### Are you the author of XnSketch?

No, I only created the flatpak package for it.

See also:

* [XnSketch forum](https://newsgroup.xnview.com/viewforum.php?f=81)

