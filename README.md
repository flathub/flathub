# Anura — Flatpak manifest for Flathub

Flatpak manifest for [Anura](https://github.com/D3M-Sudo/Anura), a screen OCR tool for Linux.

## Why this exists

[Frog](https://github.com/TenderOwl/Frog) is a simple OCR app I used and liked, but development
stopped and it was no longer maintained. I forked it as a side project to learn Python and GTK
by working on something real and small enough to actually finish.

Along the way I removed the telemetry, updated the UI to use Libadwaita, added QR code decoding,
and rewrote how screenshots are taken to go through XDG Desktop Portal. It is still a work in
progress but it works well for daily use.

## Installing

```bash
flatpak install flathub com.github.d3msudo.anura
```

## Basic usage

Open the app, click **Take a screenshot**, draw a selection around the text you want, and it
lands in your clipboard. There is also a QR tab if you need to decode a QR code from the screen.

You can drag and drop an image into the window too.

## Building locally

```bash
flatpak-builder --install --user builddir com.github.d3msudo.anura.json
```

## Issues

- Packaging problems: open an issue here
- App bugs: [github.com/D3M-Sudo/Anura/issues](https://github.com/D3M-Sudo/Anura/issues)
