# Hex Colordle

## What is Hex Colordle

It is a game where you need to find the hexadecimal value of the color shown on screen.

## Requierments

+ Python 3.11+

## How to build Flatpak

```shell
flatpak-builder --user --install --force-clean build-dir net.krafting.HexColordle.yml && flatpak run net.krafting.HexColordle
```

## Screenshots

![Program's main window](images/main_window.png)

## Generate releases:

Add content in NEWS, then launch:

```shell
appstreamcli news-to-metainfo ./NEWS ./net.krafting.HexColordle.metainfo.xml
```

And then update the version in the about page

We then update the version in the manifest locally, push everything and create a new release in gitlab and then update the version in the flathub repo.
