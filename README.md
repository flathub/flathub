# uget-flatpak

**uGet** is a lightweight yet powerful Open Source download manager for GNU/Linux developed with GTK+.

![uget-flatpak screenshot](uget-flatpak.png)

[Homepage](https://ugetdm.com)

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
$ flatpak-builder "build" "com.ugetdm.uGet.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "com.ugetdm.uGet.yaml" "sh"
```

### Test run

```
$ flatpak-builder --run "build" "com.ugetdm.uGet.yaml" "uget-gtk"
```

### Install

```
$ flatpak-builder --repo="repo" --force-clean "build" "com.ugetdm.uGet.yaml"
```

```
$ flatpak --user remote-add --no-gpg-verify "uget" "repo"
```

```
$ flatpak --user install "uget" "com.ugetdm.uGet"
```

### Run

```
$ flatpak run "com.ugetdm.uGet"
```

### Uninstall

```
$ flatpak --user uninstall "com.ugetdm.uGet"
```

```
$ flatpak --user remote-delete "uget"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Why not a RPM package?

There are already [packages](https://pkgs.org/download/uget) for various distributions. However, they may be outdated.

### Are you the author of uGet?

No, I only created the flatpak package for it.

See also:

* [SourceForge repo](https://sourceforge.net/projects/urlget)

