# xaralx-flatpak

**XaraLX** (AKA *Xara Xtreme for Linux*) is a powerful, general purpose vector graphics program.

![xaralx-flatpak screenshot](xaralx-flatpak.png)

[Homepage](http://www.xaraxtreme.org)

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
$ flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
```

See also:

* [flathub setup](http://docs.flatpak.org/en/latest/using-flatpak.html#add-a-remote)

### Prepare

```
$ flatpak install flathub org.freedesktop.Sdk//18.08
```

```
$ flatpak install flathub org.freedesktop.Platform//18.08
```

### Build

```
$ mkdir -p build && flatpak-builder "build" "org.xaraxtreme.XaraLX.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "org.xaraxtreme.XaraLX.yaml" "sh"
```

### Run

```
$ flatpak-builder --run "build" "org.xaraxtreme.XaraLX.yaml" "xaralx"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Does flatpak-ed XaraLX run as superuser?

[No](https://github.com/flatpak/flatpak/issues/1557). It is a [MATE](https://github.com/mate-desktop)/[marco](https://github.com/mate-desktop/marco) [issue](https://github.com/mate-desktop/marco/issues/301).

### Why not use an RPM package?

I already provided a (S)RPM package for EL and Fedora.

### Are you the author of XaraLX?

No, I only created the flatpak package for it.

See also:

* [XaraLX FAQ](http://www.xaraxtreme.org/faqs.html)

