<img style="vertical-align: middle;" src="https://gitlab.com/payoliin/csnake/-/raw/master/flatpak/icons/io.gitlab.payoliin.csnake.svg" width="120" height="120" align="left">

# CSnake

A simple snake implementation in your terminal

[![Get it on Flathub](https://flathub.org/api/badge?locale=en)](https://flathub.org/apps/io.gitlab.payoliin.csnake)

## How to build

1. Clone this repository
```
git clone https://github.com/flathub/io.gitlab.payoliin.csnake.git
```
2. Download the [Flatpak Builder](https://github.com/flathub/org.flatpak.Builder)
```
flatpak install org.flatpak.Builder
```
3. Execute the following command inside the repository
```bash
flatpak run org.flatpak.Builder --force-clean <build-dir> io.gitlab.payoliin.csnake.yml --install --user
```
**Command detail:**

- `--force-clean`: Needed if your build-dir is not empty
- `<build-dir>`: Path to the build directory (e.g.: `_build`)
- `io.gitlab.payoliin.csnake.yml`: Path to the Flatpak manifest
- `--install`: Needed if you want to install the app when built
- `--user`: Needed if you want the app installed in the user installation (easier for debugging purposes)