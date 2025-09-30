# io.github.DamnedAngel.msx-tile-forge

Flatpak packaging for **MSX Tile Forge**  
A tile, palette and map editor for the MSX platform.

## Upstream Project

- Source: [https://github.com/DamnedAngel/msx-tile-forge](https://github.com/DamnedAngel/msx-tile-forge)

## User Guide

### How to install and run from Flathub

```bash
flatpak install flathub io.github.DamnedAngel.msx-tile-forge
flatpak run io.github.DamnedAngel.msx-tile-forge
```

## Developer Guide

If you're new about flathub, it's a good idea to read [submission](https://docs.flathub.org/docs/for-app-authors/submission), [maintenance](https://docs.flathub.org/docs/for-app-authors/maintenance) and [updates](https://docs.flathub.org/docs/for-app-authors/updates) guides.

### How to get this repo

1. Fork the [project flathub repo](https://github.com/flathub/io.github.DamnedAngel.msx-tile-forge);

2. Create a folder in your local machine;

3. Clone the repo files:
   ```
   git clone https://github.com/<your_user_name>/io.github.DamnedAngel.msx-tile-forge.git
   ```

### How to do a local test

1. Smoke test:

   ```
   ./install.sh
   ./test.sh
   ./uninstall.sh
   ```

2. Full test (with linting):
   ```
   ./linting.sh
   ```
   Note: this flatpak requires Python3+TK+TCL, so the building process can spend a little more time than usual.

### How to upload a new release

1. Open `io.github.DamnedAngel.msx-tile-forge.metainfo.xml` and add a new `release version` row into `releases` tag;

2. Open `io.github.DamnedAngel.msx-tile-forge.yaml` and change `APP_VERSION`, `tag` and `commit`;

3. Do a full test (with linting):

   ```
   ./linting.sh
   ```

   Note: this flatpak requires Python3+TK+TCL, so the building process can spend a little more time than usual.

4. Update your repo master branch:

   ```
   git commit -m "Release <write_here_the_new_release_version>"
   git push origin master
   ```

5. Make a [PR](https://docs.flathub.org/docs/for-app-authors/updates#creating-updates) to the [project flathub repo](https://github.com/flathub/io.github.DamnedAngel.msx-tile-forge) master branch.
