# gimagereader-flatpak

**gImageReader** is a simple optical character recognition (OCR) application which acts as a frontend to the tesseract OCR engine.

![gimagereader-gtk3-flatpak screenshot](gimagereader-gtk3-flatpak.png)

[Homepage](https://github.com/manisandro/gImageReader)

This repo is about the flatpak package (Gtk+3).

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
$ flatpak install "flathub" "org.gnome.Sdk//3.30"
```

```
$ flatpak install "flathub" "org.gnome.Platform//3.30"
```

### Build

```
$ flatpak-builder "build" "com.github.manisandro.gImageReader-gtk.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "com.github.manisandro.gImageReader-gtk.yaml" "sh"
```

### Install

```
$ flatpak-builder --repo="repo" --force-clean "build" "com.github.manisandro.gImageReader-gtk.yaml"
```

```
$ flatpak --user remote-add --no-gpg-verify "gimagereader-gtk3" "repo"
```

```
$ flatpak --user install "gimagereader-gtk3" "com.github.manisandro.gImageReader-gtk"
```

### Run

```
$ flatpak run "com.github.manisandro.gImageReader-gtk"
```

### Uninstall

```
$ flatpak --user uninstall "com.github.manisandro.gImageReader-gtk"
```

```
$ flatpak --user remote-delete "gimagereader-gtk3"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Known issues

#### `gimagereader-gtk`

 * ~~Unable to hide the `Manage Languages` position from menu when using `System-wide paths` and using **PackageKit** doesn't make much sense in the sandboxed environment.~~
 * ~~Unable to manage languages when using `User paths`: `Failed to fetch list of available languages: Failed to fetch list of available languages: ` (`gio.File.read_finish`: `Operation not supported` - probably missing required dependencies for **GVFS**, maybe the `http` backend?).~~
 * ~~Unable to download dictionaries when using `User paths`: `Could note read https://cgit.freedesktop.org/libreoffice/dictionaries/tree/: .` (`gio.File.read_finish`: `Operation not supported` - probably missing required dependencies for **GVFS**, maybe the `http` backend?).~~

See also:
 * http://gtk.10911.n7.nabble.com/dynamic-menus-td93355.html
 * https://bugzilla.gnome.org/show_bug.cgi?id=791175
 * https://gitlab.gnome.org/GNOME/gtk/issues/987

### Are you the author of gImageReader?

No, I only created the flatpak package for it.

See also:

* [GitHub repo](https://github.com/manisandro/gImageReader)

