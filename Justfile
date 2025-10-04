build:
  flatpak run org.flatpak.Builder \
    --install \
    --keep-build-dirs \
    --install-deps-from=flathub \
    --force-clean \
    --verbose \
    build --user org.kde.discover.yaml

run:
  flatpak run org.kde.discover

debug:
  flatpak run --command=bash org.kde.discover

devel:
  flatpak run --command=bash --devel org.kde.discover


