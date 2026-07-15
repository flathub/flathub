# com.dygma.bazecor (Flathub Bundle)

Flathub packaging repository for **Bazecor** by Dygma.

## Build

Build this Flatpak bundle locally:

```bash
./build_and_install.sh
```

## Install From Flathub

Install Bazecor from Flathub:

```bash
flatpak install flathub com.dygma.bazecor
```

Run Bazecor:

```bash
flatpak run com.dygma.bazecor
```

Update Bazecor:

```bash
flatpak update com.dygma.bazecor
```

Remove Bazecor:

```bash
flatpak uninstall com.dygma.bazecor
```

## For Maintainers

### How to I deploy a new release version?

We have included an simple to use `update.sh` that will take care of automatically updating the `com.dygma.bazecor.yml` to update application source and resource assets to the desired version.

#### Usage

```bash
Usage:
  ./update.sh [--version <latest|VERSION-TAG>]

Examples:
  ./update.sh
  ./update.sh --version latest
  ./update.sh --version v1.9.0
```

If no `--version` parameter is provided, it will assume `latest`.

#### Required system setup

The script makes use of the following command line utilities:

- `appstreamcli`
- `awk`
- `curl`
- `flatpak`
- `flatpak-node-generator`
- `git`
- `jq`
- `mktemp`
- `sed`
- `sha256sum`
- `yq` (specifically version 4.x, available at [https://github.com/mikefarah/yq](https://github.com/mikefarah/yq))

Make sure you have them all installed on your build system.

For Debian-Based

```bash
sudo apt update
sudo apt install -y curl flatpak-builder git jq
```

For Redhat-Based

```bash
sudo dnf install -y curl flatpak-builder git jq
```

For Arch Linux Based

```bash
sudo pacman -S --needed curl flatpak-builder git jq
```

#### yq v4.x

As for `yq` v4.x, you can install it locally like so:

```bash
wget https://github.com/mikefarah/yq/releases/latest/download/yq_linux_amd64 -O ~/.local/bin/yq && chmod +x ~/.local/bin/yq
```

yq v4 is also available on homebrew if you prefer installing that way.

```bash
brew install yq
```

If you are using Ubuntu, yq v4 is also available from the snap store.

```bash
sudo snap install yq
```

#### flatpak-node-generator

As for flatpak-node-generator, it can be installed via pip

```bash
pipx install git+https://github.com/flatpak/flatpak-builder-tools.git#subdirectory=node
```

More information available here: [https://github.com/flatpak/flatpak-builder-tools/tree/master/node#usage](https://github.com/flatpak/flatpak-builder-tools/tree/master/node#usage)

### How to I test my release locally?

after running `./update.sh`, you can run `./local-test-build.sh` to verify that everything is in good shape before committing your changes and opening a PR to flathub.
