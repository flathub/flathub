# com.kjxbyz.PicGuard

[![Flathub Version](https://img.shields.io/flathub/v/com.kjxbyz.PicGuard)](https://flathub.org/apps/com.kjxbyz.PicGuard)

## Requirements

```shell
sudo apt install -y flatpak

sudo apt install -y gnome-software-plugin-flatpak

flatpak remote-add --if-not-exists --user flathub https://flathub.org/repo/flathub.flatpakrepo

flatpak install -y flathub org.flatpak.Builder

flatpak install flathub org.Gnome.Platform//46 org.Gnome.Sdk//46
```

## Build and install the app

```shell
flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --mirror-screenshots-url=https://dl.flathub.org/media/ --repo=repo build com.kjxbyz.PicGuard.yml
```

```shell
flatpak run com.kjxbyz.PicGuard
```

```shell
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest com.kjxbyz.PicGuard.yml
```

## TODO

1. Update sha256
2. Add screenshots
3. Update `Version` in `com.kjxbyz.PicGuard.desktop` file
