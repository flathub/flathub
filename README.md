Just run these commands in a terminal in this directory to run the game.

```bash
flatpak install flathub org.freedesktop.Sdk//23.08
flatpak install flathub org.freedesktop.Platform//23.08
flatpak-builder --user --install build-dir supertux-advance.json
flatpak run com.github.SuperTuxAdvance
```
