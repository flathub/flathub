# xnretro-flatpak

**XnRetro** allows you to create stunning images with vintage or instant effect on your Desktop.

![xnretro-flatpak screenshot](xnretro-flatpak.png)

[Homepage](https://www.xnview.com/en/xnretro/)

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
$ flatpak-builder "build" "com.xnview.XnRetro.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "com.xnview.XnRetro.yaml" "sh"
```

### Test run

```
$ flatpak-builder --run "build" "com.xnview.XnRetro.yaml" "xnretro"
```

### Install

```
$ flatpak-builder --repo="repo" --force-clean "build" "com.xnview.XnRetro.yaml"
```

```
$ flatpak --user remote-add --no-gpg-verify "xnretro" "repo"
```

```
$ flatpak --user install "xnretro" "com.xnview.XnRetro"
```

### Run

```
$ flatpak run "com.xnview.XnRetro"
```

### Uninstall

```
$ flatpak --user uninstall "com.xnview.XnRetro"
```

```
$ flatpak --user remote-delete "xnretro"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Does flatpak-ed XnRetro run as superuser?

[No](https://github.com/flatpak/flatpak/issues/1557). It is a [MATE](https://github.com/mate-desktop)/[marco](https://github.com/mate-desktop/marco) [issue](https://github.com/mate-desktop/marco/issues/301).

### Is this freeware?

Yes.

### Are you the author of XnRetro?

No, I only created the flatpak package for it.

See also:

* [XnRetro forum](https://newsgroup.xnview.com/viewforum.php?f=81)

