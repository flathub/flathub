# xnviewmp-flatpak

**XnView MP** is the enhanced version of XnView Classic. It is a powerful picture viewer, browser and converter for Windows, Mac and Linux. This software can read more than 500 formats change picture size, reduce picture file size and much more!

![xnviewmp-flatpak screenshot](xnviewmp-flatpak.png)

[Homepage](https://www.xnview.com/en/xnviewmp/)

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

### Build

```
$ flatpak-builder "build" "com.xnview.XnViewMP.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "com.xnview.XnViewMP.yaml" "sh"
```

### Test run

```
$ flatpak-builder --run "build" "com.xnview.XnViewMP.yaml" "xnview"
```

### Install

```
$ flatpak-builder --repo="repo" --force-clean "build" "com.xnview.XnViewMP.yaml"
```

```
$ flatpak --user remote-add --no-gpg-verify "xnviewmp" "repo"
```

```
$ flatpak --user install "xnviewmp" "com.xnview.XnViewMP"
```

### Run

```
$ flatpak run "com.xnview.XnViewMP"
```

### Uninstall

```
$ flatpak --user uninstall "com.xnview.XnViewMP"
```

```
$ flatpak --user remote-delete "xnviewmp"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Does flatpak-ed XnView MP run as superuser?

[No](https://github.com/flatpak/flatpak/issues/1557). It is a [MATE](https://github.com/mate-desktop)/[marco](https://github.com/mate-desktop/marco) [issue](https://github.com/mate-desktop/marco/issues/301).

### Is this freeware?

XnView MP is provided as FREEWARE (NO Adware, NO Spyware) for private or educational use (including non-profit organizations).
If you intend to use XnView in a company, you must purchase a [license](https://www.xnview.com/en/xnviewmp/#downloads).

### Are you the author of XnView MP?

No, I only created the flatpak package for it.

See also:

* [XnView MP forum](https://newsgroup.xnview.com/viewforum.php?f=68)

