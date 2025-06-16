# handyfox is a firefox for mobile devices

<img src="https://raw.githubusercontent.com/gmanka-flatpaks/io.github.gmankab.handyfox/refs/heads/main/modules/metainfo/screenshot.png" height="400" />

local build and install guide:

```sh
git clone https://github.com/gmanka-flatpaks/io.github.gmankab.handyfox
cd io.github.gmankab.handyfox
flatpak --user remote-add flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak install org.flatpak.Builder
flatpak run org.flatpak.Builder --user --install --install-deps-from=flathub --force-clean --repo=repo build io.github.gmankab.handyfox.yml
```

### license

- [firefox](https://github.com/mozilla-firefox/firefox) license is mpl-2.0
- [mobile-config-firefox](https://gitlab.postmarketos.org/postmarketOS/mobile-config-firefox) license is mpl-2.0
- [firefox-gnome-theme](https://github.com/rafaelmardojai/firefox-gnome-theme) license is unlicense
- [handyfox](https://github.com/gmanka-flatpaks/io.github.gmankab.handyfox) manifest and metadata license is agpl 3

