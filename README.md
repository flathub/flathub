# welle.io flatpak package repo

Please see the upstream repository https://github.com/AlbrechtL/welle.io.

## Building the flatpak

See https://docs.flathub.org/docs/for-app-authors/submission

Build
   ```
flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --mirror-screenshots-url=https://dl.flathub.org/media/ --repo=repo builddir io.welle.welle-gui.yml
   ```

Create local flatpak for testing it on another machine
   ```
flatpak build-bundle repo welle-io.flatpak io.welle.welle-gui --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo
   ```