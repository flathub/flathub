# notepadqq-flatpak

Notepadqq is a text editor designed by developers, for developers.

![notepadqq-flatpak screenshot](notepadqq-flatpak.png)

[Homepage](https://github.com/notepadqq/notepadqq)

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
$ flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
```

See also:

* [flathub setup](http://docs.flatpak.org/en/latest/using-flatpak.html#add-a-remote)

### Prepare

```
$ flatpak install flathub org.kde.Sdk//5.11
```

```
$ flatpak install flathub org.kde.Platform//5.11
```

```
$ flatpak install flathub io.qt.qtwebkit.BaseApp//5.11
```

### Build

```
$ mkdir -p build && flatpak-builder "build" "com.notepadqq.Notepadqq.yaml" --force-clean --install-deps-from="flathub"
```

### Install

```
$ flatpak-builder --repo=repo --force-clean build com.notepadqq.Notepadqq.yaml
```

```
$ flatpak --user remote-add --no-gpg-verify notepadqq repo
```

```
$ flatpak --user install notepadqq com.notepadqq.Notepadqq
```

### Run

```
$ flatpak run com.notepadqq.Notepadqq
```

### Uninstall

```
$ flatpak --user uninstall com.notepadqq.Notepadqq
```

```
$ flatpak --user remote-delete notepadqq
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Does flatpak-ed Notepadqq run as superuser?

[No](https://github.com/flatpak/flatpak/issues/1557). It is a [MATE](https://github.com/mate-desktop)/[marco](https://github.com/mate-desktop/marco) [issue](https://github.com/mate-desktop/marco/issues/301).

### Why not use an RPM package?

I already provided a [repo](https://copr.fedorainfracloud.org/coprs/scx/notepadqq/) with RPM packages.

### Why I can't open any file?

Try:

1. install the latest version of `flatpak`
2. install the `xdg-desktop-portal*` packages
3. install the *com.notepadqq.Notepadqq* as a [flatpak package](http://docs.flatpak.org/en/latest/first-build.html#install-the-app)
4. set the [XDG_RUNTIME_DIR](https://github.com/flatpak/flatpak/issues/534#issuecomment-378824515) environment variable: `XDG_RUNTIME_DIR="/run/user/${UID}"`

### Are you the author of Notepadqq?

No, I only created the flatpak package for it.

See also:

* [Notepadqq readme](https://github.com/notepadqq/notepadqq/blob/master/README.md)

