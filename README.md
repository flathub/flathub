# JA2 Stracciatella Flatpak

A [flatpak][] manifest for [JA2 Stracciatella][ja2].

[flatpak]: https://flatpak.org/
[ja2]: https://github.com/ja2-stracciatella/ja2-stracciatella

## Wayland

The launcher still requires X11, but JA2 itself can run natively on Wayland (which includes support for automatic HiDPI scaling).
To enable wayland set `$SDL_VIDEODRIVER` to `wayland`.
To do this persistently only for JA2, run the following command

```
flatpak override --user --env=SDL_VIDEODRIVER=wayland io.github.ja2-stracciatella
```
