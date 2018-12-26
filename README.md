# kchmviewer-flatpak

**KchmViewer** is a reader for CHM (WinHelp) and EPUB files using Qt toolkit with optional KDE support. The main point of kchmviewer is compatibility with non-English CHM files, including most international charsets.

![kchmviewer-flatpak screenshot](kchmviewer-flatpak.png)

[Homepage](http://www.ulduzsoft.com/linux/kchmviewer)

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
$ flatpak install "flathub" "org.kde.Sdk//5.9"
```

```
$ flatpak install "flathub" "org.kde.Platform//5.9"
```

### Build

```
$ flatpak-builder "build" "com.ulduzsoft.KchmViewer.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "com.ulduzsoft.KchmViewer.yaml" "sh"
```

### Install

```
$ flatpak-builder --repo="repo" --force-clean "build" "com.ulduzsoft.KchmViewer.yaml"
```

```
$ flatpak --user remote-add --no-gpg-verify "kchmviewer" "repo"
```

```
$ flatpak --user install "kchmviewer" "com.ulduzsoft.KchmViewer"
```

### Run

```
$ flatpak run "com.ulduzsoft.KchmViewer"
```

### Uninstall

```
$ flatpak --user uninstall "com.ulduzsoft.KchmViewer"
```

```
$ flatpak --user remote-delete "kchmviewer"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Does flatpak-ed KchmViewer run as superuser?

[No](https://github.com/flatpak/flatpak/issues/1557). It is a [MATE](https://github.com/mate-desktop)/[marco](https://github.com/mate-desktop/marco) [issue](https://github.com/mate-desktop/marco/issues/301).

### Why not a RPM package?

I already provided [COPR repo](https://copr.fedorainfracloud.org/coprs/scx/kchmviewer) with (S)RPM packages for EL.

### Are you the author of KchmViewer?

No, I only created the flatpak package for it.

See also:

* [SourceForge repo](https://sourceforge.net/projects/kchmviewer)

