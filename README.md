# OpenNumismat

OpenNumismat is a software for management custom coin collection catalog.

This repo hosts the flatpak wrapper for [OpenNumismat](https://opennumismat.github.io/), available at [Flathub](https://flathub.org/apps/io.github.opennumismat.OpenNumismat).

```sh
flatpak install flathub io.github.opennumismat.OpenNumismat
flatpak run io.github.opennumismat.OpenNumismat
```

### Wayland

This package uses the flags to run on Wayland. To opt-out it run:

```sh
flatpak override --user --nosocket=wayland io.github.opennumismat.OpenNumismat
```
