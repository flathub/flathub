# Total Chaos - Retro Edition

This project contains files to build [Total Chaos - Retro Edition](https://www.moddb.com/mods/total-chaos/downloads/total-chaos-directors-cut-retro-edition-140) as flatpak app.

Retro Edition has reduced texture resolutions and poly-counts for improved performance. Save files from other versions of Total Chaos are not compatible.

The game runs using the GZDoom Legacy v3.8.2 engine to ensure more compatibility with old computers. It requires OpenGL 2 or later and at least 1GB of free RAM.

This project is based on [com.moddb.TotalChaos](https://github.com/flathub/com.moddb.TotalChaos).

### Tips for gaming
- Skip the intro by pressing the `E` button
- Reduce pixelation by disabling `OPTIONS > Post Processing > RETRO`

### GZDoom options
Defaults for the GZDoom engine are in file `~/.var/app/com.moddb.TotalChaosRetro/.config/gzdoom/gzdoom.ini`

|  Option       | Descritipn                                              |
|---------------|---------------------------------------------------------|
| `vid_adapter` | Sets the primary display (when using multiple monitors) |
