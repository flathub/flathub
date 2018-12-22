# moeditor-flatpak

**Moeditor** is all-purpose markdown editor.

![moeditor-flatpak screenshot](moeditor-flatpak.png)

[Homepage](https://moeditor.js.org)

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
$ flatpak-builder "build" "com.github.Moeditor.Moeditor.yaml" --force-clean --install-deps-from="flathub"
```

### Test

```
$ flatpak-builder --run "build" "com.github.Moeditor.Moeditor.yaml" "sh"
```

### Test run

```
$ flatpak-builder --run "build" "com.github.Moeditor.Moeditor.yaml" "Moeditor"
```

### Install

```
$ flatpak-builder --repo="repo" --force-clean "build" "com.github.Moeditor.Moeditor.yaml"
```

```
$ flatpak --user remote-add --no-gpg-verify "moeditor" "repo"
```

```
$ flatpak --user install "moeditor" "com.github.Moeditor.Moeditor"
```

### Run

```
$ flatpak run "com.github.Moeditor.Moeditor"
```

### Uninstall

```
$ flatpak --user uninstall "com.github.Moeditor.Moeditor"
```

```
$ flatpak --user remote-delete "moeditor"
```

See also: [Building your first Flatpak](http://docs.flatpak.org/en/latest/first-build.html)

## FAQ

### Are you the author of Moeditor?

No, I only created the flatpak package for it.

See also:

* [GitHub repo](https://github.com/Moeditor/Moeditor)

