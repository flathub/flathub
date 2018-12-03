# speedcrunch-flatpak

**SpeedCrunch** is a high-precision scientific calculator featuring a fast, keyboard-driven user interface.

![speedcrunch-flatpak screenshot](speedcrunch-flatpak.png)

[Homepage](http://speedcrunch.org)

This repo is about flatpak package.

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
$ mkdir -p "build" && flatpak-builder "build" "org.speedcrunch.SpeedCrunch.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "org.speedcrunch.SpeedCrunch.yaml" "sh"
```

### Test run

```
$ flatpak-builder --run "build" "org.speedcrunch.SpeedCrunch.yaml" "speedcrunch"
```

### Install

```
$ flatpak-builder --repo="repo" --force-clean "build" "org.speedcrunch.SpeedCrunch.yaml"
```

```
$ flatpak --user remote-add --no-gpg-verify "speedcrunch" "repo"
```

```
$ flatpak --user install "speedcrunch" "org.speedcrunch.SpeedCrunch"
```

### Run

```
$ flatpak run "org.speedcrunch.SpeedCrunch"
```

### Uninstall

```
$ flatpak --user uninstall "org.speedcrunch.SpeedCrunch"
```

```
$ flatpak --user remote-delete "speedcrunch"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Does flatpak-ed SpeedCrunch run as superuser?

[No](https://github.com/flatpak/flatpak/issues/1557). It is a [MATE](https://github.com/mate-desktop)/[marco](https://github.com/mate-desktop/marco) [issue](https://github.com/mate-desktop/marco/issues/301).

### Why not use an RPM package?

I already provided a [repo](https://copr.fedorainfracloud.org/coprs/scx/speedcrunch/) with RPM packages.

### Are you the author of SpeedCrunch?

No, I only created the flatpak package for it.

See also:

* [SpeedCrunch repo](https://bitbucket.org/heldercorreia/speedcrunch/overview)

