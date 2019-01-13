# gimagereader-flatpak

**gImageReader** is a simple optical character recognition (OCR) application which acts as a frontend to the tesseract OCR engine.

![gimagereader-qt5-flatpak screenshot](gimagereader-qt5-flatpak.png)

![gimagereader-qt5-gnome-flatpak screenshot](gimagereader-qt5-gnome-flatpak.png)

[Homepage](https://github.com/manisandro/gImageReader)

This repo is about the flatpak package (Qt5).

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
$ flatpak install "flathub" "org.kde.Sdk//5.11"
```

```
$ flatpak install "flathub" "org.kde.Platform//5.11"
```

### Build

```
$ flatpak-builder "build" "com.github.manisandro.gImageReader-qt5.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "com.github.manisandro.gImageReader-qt5.yaml" "sh"
```

### Install

```
$ flatpak-builder --repo="repo" --force-clean "build" "com.github.manisandro.gImageReader-qt5.yaml"
```

```
$ flatpak --user remote-add --no-gpg-verify "gimagereader-qt5" "repo"
```

```
$ flatpak --user install "gimagereader-qt5" "com.github.manisandro.gImageReader-qt5"
```

### Run

```
$ flatpak run "com.github.manisandro.gImageReader-qt5"
```

### Uninstall

```
$ flatpak --user uninstall "com.github.manisandro.gImageReader-qt5"
```

```
$ flatpak --user remote-delete "gimagereader-qt5"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Are you the author of gImageReader?

No, I only created the flatpak package for it.

See also:

* [GitHub repo](https://github.com/manisandro/gImageReader)

