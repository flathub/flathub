# pinta-flatpak

**Pinta** is a free, open source program for drawing and image editing.

![pinta-flatpak screenshot](pinta-flatpak.png)

[Homepage](https://pinta-project.com/pintaproject/pinta)

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
$ flatpak install "flathub" "org.gnome.Sdk//3.28"
```

```
$ flatpak install "flathub" "org.gnome.Platform//3.28"
```

```
$ git submodule init
```

```
$ git submodule update
```

### Build

```
$ mkdir -p "build" && flatpak-builder "build" "com.github.PintaProject.Pinta.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "com.github.PintaProject.Pinta.yaml" "sh"
```

### Test run

```
$ flatpak-builder --run "build" "com.github.PintaProject.Pinta.yaml" "pinta"
```

### Install

```
$ flatpak-builder --repo="repo" --force-clean "build" "com.github.PintaProject.Pinta.yaml"
```

```
$ flatpak --user remote-add --no-gpg-verify "pinta" "repo"
```

```
$ flatpak --user install "pinta" "com.github.PintaProject.Pinta"
```

### Run

```
$ flatpak run "com.github.PintaProject.Pinta"
```

### Uninstall

```
$ flatpak --user uninstall "com.github.PintaProject.Pinta"
```

```
$ flatpak --user remote-delete "pinta"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Does flatpak-ed Pinta run as superuser?

[No](https://github.com/flatpak/flatpak/issues/1557). It is a [MATE](https://github.com/mate-desktop)/[marco](https://github.com/mate-desktop/marco) [issue](https://github.com/mate-desktop/marco/issues/301).

### Why not create a RPM package?

There are already [packages](https://pkgs.org/download/pinta) for various distributions.

### Are you the author of Pinta?

No, I only created the flatpak package for it.

See also:

* [GitHub repo](https://github.com/PintaProject/Pinta)

