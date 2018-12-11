# hardinfo-flatpak

**HardInfo** is a system profiler and benchmark for Linux systems.

![hardinfo-flatpak screenshot](hardinfo-flatpak.png)

[Homepage](http://hardinfo.org)

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
$ flatpak-builder "build" "com.github.lpereira.hardinfo.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "com.github.lpereira.hardinfo.yaml" "sh"
```

### Test run

```
$ flatpak-builder --run "build" "com.github.lpereira.hardinfo.yaml" "hardinfo"
```

### Install

```
$ flatpak-builder --repo="repo" --force-clean "build" "com.github.lpereira.hardinfo.yaml"
```

```
$ flatpak --user remote-add --no-gpg-verify "hardinfo" "repo"
```

```
$ flatpak --user install "hardinfo" "com.github.lpereira.hardinfo"
```

### Run

```
$ flatpak run "com.github.lpereira.hardinfo"
```

### Uninstall

```
$ flatpak --user uninstall "com.github.lpereira.hardinfo"
```

```
$ flatpak --user remote-delete "hardinfo"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Does flatpak-ed HardInfo run as superuser?

[No](https://github.com/flatpak/flatpak/issues/1557). It is a [MATE](https://github.com/mate-desktop)/[marco](https://github.com/mate-desktop/marco) [issue](https://github.com/mate-desktop/marco/issues/301).

### Why not use an RPM package?

There are already [packages](https://pkgs.org/download/hardinfo) for various distributions.

### Is this package fully functional?

Unfortunately, no. Sandbox limits functionality. However, this package still offers a large number of functions.

### Are you the author of HardInfo?

No, I only created the flatpak package for it.

See also:

* [GitHub repo](https://github.com/lpereira/hardinfo)

