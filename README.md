Just run these commands in a terminal in this directory to run the game.

```bash
flatpak install flathub org.freedesktop.Sdk//24.08
flatpak install flathub org.freedesktop.Platform//24.08
flatpak-builder --user --install build-dir io.github.JaredTweed.SuperTuxAdvance.json
flatpak run io.github.JaredTweed.SuperTuxAdvance
```
