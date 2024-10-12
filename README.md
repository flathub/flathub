# 📦 TerraTactician-Expandoria Flatpak Manifest Source

## 🧰 Building
This guide is mostly taken from the [Build your first Flatpak Guide](https://docs.flatpak.org/en/latest/first-build.html).

First of all, 
make sure you have `flatpak-builder` and `appstream-compose` installed.
You can probably get them from your system repository.

Make sure that you have cloned the *flatpak manifest repo* (this repo) and navigate into it:
```sh
git clone https://codeberg.org/terratactician_expandoria/flatpak
cd flatpak
```

If you have never used flatpak before, you will most likely have to add the default *flathub* remote
```sh
flatpak remote-add --user --if-not-exists flathub "https://flathub.org/repo/flathub.flatpakrepo"
```

To build and install the flatpak, you can use the following command.
If you do not want to install it (i.e. because you plan on adding it to a repository), remove the `--install` flag.
```sh
flatpak-builder --user --install-deps-from=flathub --install --force-clean build page.codeberg.terratactician_expandoria.game.yaml
```

After installing the flatpak, you should be able to run it using the CLI, or by using the desktop entry.
```sh
flatpak --user run page.codeberg.terratactician_expandoria.game
```


## 🆕 Updating
When the `Cargo.lock` changes, the `cargo-sources.json` has to be regenerated:
```sh
python flatpak-cargo-generator.py ../game/Cargo.lock -o cargo-sources.json
```

To update the application to a new upstream version, simply change the `version` number
