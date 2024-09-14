# com.chenyifaer.FaFaRunner

[![Flathub Version](https://img.shields.io/flathub/v/com.chenyifaer.FaFaRunner)](https://flathub.org/apps/com.chenyifaer.FaFaRunner)

## Requirements

```shell
sudo apt install -y flatpak

sudo apt install -y gnome-software-plugin-flatpak

flatpak remote-add --if-not-exists --user flathub https://flathub.org/repo/flathub.flatpakrepo

flatpak install -y flathub org.flatpak.Builder

flatpak install flathub org.Gnome.Platform//46 org.Gnome.Sdk//46

flatpak install flathub org.freedesktop.Platform//23.08 org.freedesktop.Sdk//23.08
```

## Build and install the app

```shell
flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --mirror-screenshots-url=https://dl.flathub.org/media/ --repo=repo build com.chenyifaer.FaFaRunner.yml
```

```shell
flatpak run com.chenyifaer.FaFaRunner
```

```shell
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest com.chenyifaer.FaFaRunner.yml
```
