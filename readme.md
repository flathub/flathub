# handyfox is a firefox for mobile devices

local build and install guide:

```sh
git clone https://github.com/gmanka-flatpaks/io.github.gmankab.handyfox
cd io.github.gmankab.handyfox
flatpak --user remote-add flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak install org.flatpak.Builder
flatpak run org.flatpak.Builder --user --install --install-deps-from=flathub --force-clean --repo=repo build io.github.gmankab.handyfox.yml
```

