# ocrfeeder-flatpak

**OCRFeeder** is a complete Optical Character Recognition and Document Analysis and Recognition program.

![ocrfeeder-flatpak screenshot](ocrfeeder-flatpak.png)

[Homepage](https://wiki.gnome.org/Apps/OCRFeeder)

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
$ flatpak install "flathub" "org.gnome.Sdk//3.30"
```

```
$ flatpak install "flathub" "org.gnome.Platform//3.30"
```

```
$ git submodule init
```

```
$ git submodule update
```

### Build

```
$ flatpak-builder "build" "org.gnome.OCRFeeder.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "org.gnome.OCRFeeder.yaml" "sh"
```

### Test run

```
$ flatpak-builder --run "build" "org.gnome.OCRFeeder.yaml" "ocrfeeder"
```

### Install

```
$ flatpak-builder --repo="repo" --force-clean "build" "org.gnome.OCRFeeder.yaml"
```

```
$ flatpak --user remote-add --no-gpg-verify "ocrfeeder" "repo"
```

```
$ flatpak --user install "ocrfeeder" "org.gnome.OCRFeeder"
```

### Run

```
$ flatpak run "org.gnome.OCRFeeder"
```

### Uninstall

```
$ flatpak --user uninstall "org.gnome.OCRFeeder"
```

```
$ flatpak --user remote-delete "ocrfeeder"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Why not a RPM package?

I already provided [COPR repo](https://copr.fedorainfracloud.org/coprs/scx/ocrfeeder) with (S)RPM packages for EL and Fedora.

### Are you the author of OCRFeeder?

No, I only created the flatpak package for it.

See also:

* [GitLab repo](https://gitlab.gnome.org/GNOME/ocrfeeder)

