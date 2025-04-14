# OpenNumismat

OpenNumismat is a software for management custom coin collection catalog.

This repo hosts the flatpak wrapper for [OpenNumismat](https://opennumismat.github.io/), available at [Flathub](https://flathub.org/apps/io.github.opennumismat.open-numismat).

```sh
flatpak install flathub io.github.opennumismat.open-numismat
flatpak run io.github.opennumismat.open-numismat
```

### Wayland

This package uses the flags to run on Wayland. To opt-out it run:

```sh
flatpak override --user --nosocket=wayland io.github.opennumismat.open-numismat
```
