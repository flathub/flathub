# wxmaxima-flatpak

**wxMaxima** is a graphical user interface for the computer algebra system Maxima: a program that solves mathematical problems by manipulating equations (and outputting the resulting formula), instead of just calculating a number.

![wxmaxima-flatpak screenshot](wxmaxima-flatpak.png)

[Homepage](https://wxmaxima-developers.github.io/wxmaxima)

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
$ flatpak-builder "build" "io.github.wxmaxima_developers.wxMaxima.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "io.github.wxmaxima_developers.wxMaxima.yaml" "sh"
```

### Test run

```
$ flatpak-builder --run "build" "io.github.wxmaxima_developers.wxMaxima.yaml" "wxmaxima"
```

### Install

```
$ flatpak-builder --repo="repo" --force-clean "build" "io.github.wxmaxima_developers.wxMaxima.yaml"
```

```
$ flatpak --user remote-add --no-gpg-verify "wxmaxima" "repo"
```

```
$ flatpak --user install "wxmaxima" "io.github.wxmaxima_developers.wxMaxima"
```

### Run

```
$ flatpak run "io.github.wxmaxima_developers.wxMaxima"
```

### Uninstall

```
$ flatpak --user uninstall "io.github.wxmaxima_developers.wxMaxima"
```

```
$ flatpak --user remote-delete "wxmaxima"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Why not a RPM package?

I already provided [COPR repo](https://copr.fedorainfracloud.org/coprs/scx/wxMaxima) with (S)RPM packages for EL.

### Are you the author of wxMaxima?

No, I only created the flatpak package for it.

See also:

* [GitHub repo](https://github.com/wxMaxima-developers/wxmaxima)

