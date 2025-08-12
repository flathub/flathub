# Flathub manifest for Threadbare

- For the game source code, issue tracking, etc., see
  [@endlessm/threadbare](https://github.com/endlessm/threadbare)

Building this Flatpak requires Git LFS. If you use `org.flatpak.Builder` from
Flathub, it is patched to use Git LFS automatically. Otherwise you will need to
install and configure it yourself.

## Permissions

- `--device=all`: Currently necessary for controller support.
- `--talk-name=org.freedesktop.ScreenSaver`: Allows the game to inhibit idle.
  [godotengine/godot#108634](https://github.com/godotengine/godot/issues/108634)
