# hexoeditor-flatpak

**HexoEditor** is a markdown editor for Hexo.

![hexoeditor-flatpak screenshot](hexoeditor-flatpak.png)

[Homepage](https://github.com/zhuzhuyule/HexoEditor)

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

### Build

```
$ flatpak-builder "build" "com.github.zhuzhuyule.HexoEditor.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "com.github.zhuzhuyule.HexoEditor.yaml" "sh"
```

### Test run

```
$ flatpak-builder --run "build" "com.github.zhuzhuyule.HexoEditor.yaml" "HexoEditor"
```

### Install

```
$ flatpak-builder --repo="repo" --force-clean "build" "com.github.zhuzhuyule.HexoEditor.yaml"
```

```
$ flatpak --user remote-add --no-gpg-verify "hexoeditor" "repo"
```

```
$ flatpak --user install "hexoeditor" "com.github.zhuzhuyule.HexoEditor"
```

### Run

```
$ flatpak run "com.github.zhuzhuyule.HexoEditor"
```

### Uninstall

```
$ flatpak --user uninstall "com.github.zhuzhuyule.HexoEditor"
```

```
$ flatpak --user remote-delete "hexoeditor"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Are you the author of HexoEditor?

No, I only created the flatpak package for it.

See also:

* [GitHub repo](https://github.com/zhuzhuyule/HexoEditor)

