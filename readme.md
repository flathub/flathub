## neovide flatpak

### building

```
git clone https://github.com/gmanka-flatpaks/dev.neovide.neovide
cd dev.neovide.neovide
flatpak run org.flatpak.Builder ./build --install-deps-from=flathub --install --force-clean --user dev.neovide.neovide.yml
```

### documentation

please refer to [documentation](doc/flatpak.txt) if you want to run terminal on your host system, use neovim from host, toolbox or distrobox

also you can get it by running `:help flatpak`

### special thanks to

- @sqwxl for writing [manifest](https://github.com/sqwxl/flathub/tree/dev.neovide.neovide)
- @1player for [host-spawn](https://github.com/1player/host-spawn)
- neovide developers
- neovim developers

