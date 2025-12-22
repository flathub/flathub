# io.github.mudkipworld.pngtuber-remix

## Local Test

```sh
flatpak-builder \
    --default-branch=localbuild \
    --force-clean \
    --repo=./repo-dir \
    ./build-dir \
    io.github.mudkipworld.pngtuber-remix.yml 

flatpak build-bundle \
    --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo \
    ./repo-dir \
    pngtuber-remix.flatpak \
    io.github.mudkipworld.pngtuber-remix \
    localbuild
```

Then install the resulting file

```sh
flatpak -y install pngtuber-remix.flatpak
```
