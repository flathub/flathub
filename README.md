# rawtherapee-flatpak

**RawTherapee** is a powerful, cross-platform raw photo processing program.

![rawtherapee-flatpak screenshot](rawtherapee-flatpak.png)

[Homepage](https://rawtherapee.com)

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

### Build

```
$ flatpak-builder "build" "com.rawtherapee.RawTherapee.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "com.rawtherapee.RawTherapee.yaml" "sh"
```

### Test run

```
$ flatpak-builder --run "build" "com.rawtherapee.RawTherapee.yaml" "rawtherapee"
```

### Install

```
$ flatpak-builder --repo="repo" --force-clean "build" "com.rawtherapee.RawTherapee.yaml"
```

```
$ flatpak --user remote-add --no-gpg-verify "rawtherapee" "repo"
```

```
$ flatpak --user install "rawtherapee" "com.rawtherapee.RawTherapee"
```

### Run

```
$ flatpak run "com.rawtherapee.RawTherapee"
```

### Uninstall

```
$ flatpak --user uninstall "com.rawtherapee.RawTherapee"
```

```
$ flatpak --user remote-delete "rawtherapee"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Why not a RPM package?

I already provided [COPR repo](https://copr.fedorainfracloud.org/coprs/scx/rawtherapee) with (S)RPM packages for EL.

### Are you the author of RawTherapee?

No, I only created the flatpak package for it.

See also:

* [GitHub repo](https://github.com/Beep6581/RawTherapee)

