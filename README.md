# qalculate-flatpak

**Qalculate!** is a multi-purpose cross-platform desktop calculator. It is simple to use but provides power and versatility normally reserved for complicated math packages, as well as useful tools for everyday needs (such as currency conversion and percent calculation). Features include a large library of customizable functions, unit calculations and conversion, symbolic calculations (including integrals and equations), arbitrary precision, uncertainty propagation, interval arithmetic, plotting, and a user-friendly interface (GTK+ and CLI).

![qalculate-flatpak screenshot](qalculate-flatpak.png)

[Homepage](http://qalculate.github.io)

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
$ flatpak-builder "build" "io.github.qalculate.Qalculate.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "io.github.qalculate.Qalculate.yaml" "sh"
```

### Test run

```
$ flatpak-builder --run "build" "io.github.qalculate.Qalculate.yaml" "qalculate-gtk"
```

### Install

```
$ flatpak-builder --repo="repo" --force-clean "build" "io.github.qalculate.Qalculate.yaml"
```

```
$ flatpak --user remote-add --no-gpg-verify "qalculate-gtk" "repo"
```

```
$ flatpak --user install "qalculate-gtk" "io.github.qalculate.Qalculate"
```

### Run

```
$ flatpak run "io.github.qalculate.Qalculate"
```

### Uninstall

```
$ flatpak --user uninstall "io.github.qalculate.Qalculate"
```

```
$ flatpak --user remote-delete "qalculate-gtk"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Are you the author of Qalculate/qalculate-gtk?

No, I only created the flatpak package for it.

See also:

* [GitHub repo](https://github.com/Qalculate/qalculate-gtk)

