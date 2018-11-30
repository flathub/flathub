# retext-flatpak

**ReText** is a simple but powerful text editor for Markdown and reStructuredText.

![retext-flatpak screenshot](retext-flatpak.png)

[Homepage](https://github.com/retext-project/retext)

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
$ flatpak install "flathub" "org.kde.Sdk//5.11"
```

```
$ flatpak install "flathub" "org.kde.Platform//5.11"
```

### Build

```
$ mkdir -p "build" && flatpak-builder "build" "me.mitya57.ReText.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "me.mitya57.ReText.yaml" "sh"
```

### Install

```
$ flatpak-builder --repo="repo" --force-clean "build" "me.mitya57.ReText.yaml"
```

```
$ flatpak --user remote-add --no-gpg-verify "retext" "repo"
```

```
$ flatpak --user install "retext" "me.mitya57.ReText"
```

### Run

```
$ flatpak run "me.mitya57.ReText"
```

### Uninstall

```
$ flatpak --user uninstall "me.mitya57.ReText"
```

```
$ flatpak --user remote-delete "retext"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Does flatpak-ed ReText run as superuser?

[No](https://github.com/flatpak/flatpak/issues/1557). It is a [MATE](https://github.com/mate-desktop)/[marco](https://github.com/mate-desktop/marco) [issue](https://github.com/mate-desktop/marco/issues/301).

### Why not use an RPM package?

This is not always possible. For example, for EL7:

* Main repo does not provide the **python3** package. However, there is a **python34** in EPEL7.
* System (main repo + EPEL) does not provide following packages: **python3-enchant**, **python3-docutils**, **python3-textile**, **python3-markups**. However, it is possible to rebuild them for EPEL7.
* System does not provide the **python3-qt5** package.
* **python3-qt5** package depends on **python3-sip** package. The **sip** package is available in EL7, but without **python3** support.
* **python-qt5** requires **sip** >= *4.18*, but EL7 provides only *4.14.6* version. The **sip** package in EL7 has not been updated since the system was released (in 2014).

### How to create module manifest for a PIP package?

You can use [Flatpak PIP Generator](https://github.com/flatpak/flatpak-builder-tools/tree/master/pip) from [Flatpak Builder Tools](https://github.com/flatpak/flatpak-builder-tools) repository.

Please remember to enable **rh-python36** on EL7.

```
scl enable rh-python36 bash
```

### Are you the author of ReText?

No, I only created the flatpak package for it.

See also:

* [ReText readme](https://github.com/retext-project/retext/blob/master/README.md)

