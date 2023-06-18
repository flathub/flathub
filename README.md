# Tambourine Music Player

Official Flathub definitions for https://github.com/MMarco94/tambourine-music-player

## Building locally

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
