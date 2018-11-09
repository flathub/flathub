# F-Droid Repomaker's Flatpak

This project contains the source files to create the
[FlatPak](https://flatpak.org/) for
[F-Droid Repomaker](https://f-droid.org/repomaker/).

With Repomaker, you can easily create your own
[F-Droid](https://f-droid.org) repo without needing any special
knowledge to do so.

## Installation

To install Repomaker through Flathub, use the following:
```
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user -y flathub org.fdroid.Repomaker
```

## Run

To run Repomaker through Flatpak, execute:
```
flatpak run org.fdroid.Repomaker
```

## Update

To update Repomaker through Flathub, use the following command:
```
flatpak update --user org.fdroid.Repomaker
```

## Development

To test the application locally, use
[flatpak-builder](http://docs.flatpak.org/en/latest/flatpak-builder.html)
with:
```
git clone https://gitlab.com/fdroid/fdroid-repomaker-flatpak.git
cd fdroid-repomaker-flatpak
git submodule update --init
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak-builder builddir --install-deps-from=flathub --user --install --force-clean org.fdroid.Repomaker.json
flatpak run org.fdroid.Repomaker --verbose
```

### Clean

```
flatpak uninstall --user org.fdroid.Repomaker
rm -rf .flatpak-builder
```

## License

Everything in this repo is licensed under GNU Affero General Public
License version 3.
See [LICENSE.md](LICENSE.md) for more information.
