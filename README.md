# xnconvert-flatpak

**XnConvert** is a powerful and free cross-platform batch image processing, allowing you to combine over 80 actions. Compatible with 500 formats. It uses the batch processing module of XnViewMP.

![xnconvert-flatpak screenshot](xnconvert-flatpak.png)

[Homepage](https://www.xnview.com/en/xnconvert/)

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
$ flatpak install "flathub" "org.freedesktop.Sdk//18.08"
```

```
$ flatpak install "flathub" "org.freedesktop.Platform//18.08"
```

### Build

```
$ flatpak-builder "build" "com.xnview.XnConvert.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "com.xnview.XnConvert.yaml" "sh"
```

### Test run

```
$ flatpak-builder --run "build" "com.xnview.XnConvert.yaml" "xnconvert"
```

### Install

```
$ flatpak-builder --repo="repo" --force-clean "build" "com.xnview.XnConvert.yaml"
```

```
$ flatpak --user remote-add --no-gpg-verify "xnconvert" "repo"
```

```
$ flatpak --user install "xnconvert" "com.xnview.XnConvert"
```

### Run

```
$ flatpak run "com.xnview.XnConvert"
```

### Uninstall

```
$ flatpak --user uninstall "com.xnview.XnConvert"
```

```
$ flatpak --user remote-delete "xnconvert"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Does flatpak-ed XnConvert run as superuser?

[No](https://github.com/flatpak/flatpak/issues/1557). It is a [MATE](https://github.com/mate-desktop)/[marco](https://github.com/mate-desktop/marco) [issue](https://github.com/mate-desktop/marco/issues/301).

### Is this freeware?

XnConvert is provided as FREEWARE (NO Adware, NO Spyware). 
If you enjoy using XnConvert, Feel free to help the developer with a small [donation](https://www.xnview.com/en/xnconvert/#downloads).

### Are you the author of XnConvert?

No, I only created the flatpak package for it.

See also:

* [XnConvert forum](https://newsgroup.xnview.com/viewforum.php?f=79)

