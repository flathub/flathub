# Hex Colordle

## What is Hex Colordle

It is a game where you need to find the hexadecimal value of the color shown on screen.

## How to build Flatpak

```shell
flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --mirror-screenshots-url=https://dl.flathub.org/media/ --repo=repo build-dir net.krafting.HexColordle.yml  && flatpak run net.krafting.HexColordle
```
