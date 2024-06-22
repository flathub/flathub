# com.chenyifaer.FaForever

## Requirements

### Add or Update the submodule

```shell
git submodule add https://github.com/flathub/shared-modules.git

git submodule update --remote --merge --init

git submodule update --remote --merge
```

### Remove the submodule

```shell
git submodule deinit -f -- shared-modules
rm -rf .git/modules/shared-modules
git rm -f shared-modules
rm .gitmodules
```

### Install flatpak

```shell
sudo apt install -y flatpak

sudo apt install -y gnome-software-plugin-flatpak

flatpak remote-add --if-not-exists --user flathub https://flathub.org/repo/flathub.flatpakrepo

flatpak install -y flathub org.flatpak.Builder

flatpak install flathub org.Gnome.Platform//46 org.Gnome.Sdk//46
```

## Build and install the app

```shell
flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --mirror-screenshots-url=https://dl.flathub.org/media/ --repo=repo build com.chenyifaer.FaForever.yml
```

```shell
flatpak run com.chenyifaer.FaForever
```

```shell
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest com.chenyifaer.FaForever.yml
```
