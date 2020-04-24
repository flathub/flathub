# Flatpak for Godot Engine

## Installation

This Flatpak is available on
[Flathub](https://flathub.org/apps/details/org.godotengine.Godot).
After following the [Flatpak setup guide](https://flatpak.org/setup/),
you can install it by entering the following command in a terminal:

```bash
flatpak install --user flathub org.godotengine.Godot -y
```

Once the Flatpak is installed, you can run Godot using your desktop environment's
application launcher.

## Updating

This Flatpak follows the latest stable Godot version.
To update it, run the following command in a terminal:

```bash
flatpak update
```

## Limitations

- No C#/Mono support
  ([#8](https://github.com/flathub/org.godotengine.Godot/issues/8)).
- No support for external script editors
  ([#55](https://github.com/flathub/org.godotengine.Godot/issues/55)).

## Building from source

Install Git, follow the
[flatpak-builder setup guide](https://docs.flatpak.org/en/latest/first-build.html)
then enter the following commands in a terminal:

```bash
git clone --recursive https://github.com/flathub/org.godotengine.Godot.git
cd org.godotengine.Godot/
flatpak install --user flathub org.freedesktop.Sdk//19.08 -y
flatpak-builder --force-clean --install --user -y builddir org.godotengine.Godot.yaml
```

If all goes well, the Flatpak will be installed after building. You can then
run it using your desktop environment's application launcher.

You can speed up incremental builds by installing [ccache](https://ccache.dev/)
and specifying `--ccache` in the flatpak-builder command line (before `builddir`).
