# Build the App

```shell
flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --mirror-screenshots-url=https://dl.flathub.org/media/ --repo=repo build-dir net.krafting.PedantiK.Lang.English.yml  && flatpak run net.krafting.PedantiK
```