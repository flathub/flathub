# Tambourine Music Player

Official Flathub definitions for https://github.com/MMarco94/tambourine-music-player

## Building locally

Validate the metadata files:
```
flatpak run org.flathub.flatpak-external-data-checker io.github.mmarco94.tambourine.yml
flatpak run --command=flatpak-builder-lint org.flatpak.Builder --exceptions io.github.mmarco94.tambourine.yml
flatpak run --env=G_DEBUG=fatal-criticals --command=appstream-util org.flatpak.Builder validate io.github.mmarco94.tambourine.metainfo.xml
```

Build the app:
```
flatpak-builder --repo=repo --force-clean build-dir io.github.mmarco94.tambourine.yml
```

Add it to your local Flatpak installation:
```
flatpak build-update-repo repo
```

Install it:
```
flatpak-builder --user --install --force-clean build-dir io.github.mmarco94.tambourine.yml
```

