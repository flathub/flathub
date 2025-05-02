# com.kjxbyz.PicGuard.Pro

[![Flathub Version](https://img.shields.io/flathub/v/com.kjxbyz.PicGuard.Pro)](https://flathub.org/apps/com.kjxbyz.PicGuard.Pro)

## Install Flatpak

```shell
sudo apt install -y flatpak
```

## Install the Software Flatpak plugin

```shell
sudo apt install -y gnome-software-plugin-flatpak
```

## Add the Flathub repository

```shell
# stable
flatpak remote-add --if-not-exists --user flathub https://flathub.org/repo/flathub.flatpakrepo

# beta
flatpak remote-add --if-not-exists flathub-beta https://flathub.org/beta-repo/flathub-beta.flatpakrepo
```

## Before submission

```shell
flatpak install -y flathub org.flatpak.Builder

flatpak install flathub org.freedesktop.Platform//23.08 org.freedesktop.Sdk//23.08
```

## Build and install

```shell
flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --mirror-screenshots-url=https://dl.flathub.org/media/ --repo=repo build com.kjxbyz.PicGuard.Pro.yml
```

```shell
flatpak run com.kjxbyz.PicGuard.Pro
```

```shell
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest com.kjxbyz.PicGuard.Pro.yml
```
