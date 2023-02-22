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

**Looking to package a Godot project as a Flatpak ?**
See [flathub/org.godotengine.godot.BaseApp](https://github.com/flathub/org.godotengine.godot.BaseApp).

## Updating

This Flatpak follows the latest stable Godot version.
To update it, run the following command in a terminal:

```bash
flatpak update
```

## Using an external script editor

This version of Godot is built with special [permissions](https://github.com/flathub/org.godotengine.Godot/blob/394f81c3310b82f5069ea917bb21f49888f818c6/org.godotengine.Godot.yaml#L46) to be able to run commands on the host system outside of the sandbox via [flatpak-spawn](https://docs.flatpak.org/en/latest/flatpak-command-reference.html#flatpak-spawn). This is done by prefixing the command with `flatpak-spawn --host`. For example, if you want to run `gnome-terminal` on the host system outside of the sandbox, you can do so by running `flatpak-spawn --host gnome-terminal`.

To spawn an external editor in Godot, all command line arguments must be split from the commands path in the [external editor preferences](https://docs.godotengine.org/en/latest/getting_started/editor/external_editor.html) and because the command needs to be prefixed with `"flatpak-spawn --host"`, the **Exec Path** is replaced by `flatpak-spawn` and the **Exec Flags** are prefixed by `--host [command path]`.

For example, for Visual Studio Code, where your [external editor preferences](https://docs.godotengine.org/en/3.2/getting_started/editor/external_editor.html) would *normally* look like this...

```text
Exec Path:  code
Exec Flags: --reuse-window {project} --goto {file}:{line}:{col}
```

...it should look like this **inside the Flatpak sandbox**:

```text
Exec Path:  flatpak-spawn
Exec Flags: --host code --reuse-window {project} --goto {file}:{line}:{col}
```

## C# Support

After the first C# script is created, it is necessary to click `build` within Godot. Alternatively, running `dotnet restore` in the IDE's terminal would work as well.

It is also necessary to add the fallback folder for other flatpaks that use .NET 6. This can be done like so:
```
flatpak --user override --filesystem=~/.var/app/org.godotengine.Godot/data/godot/mono/GodotNuGetFallbackFolder
```

## Limitations

- The Mono external editor support does not work.

## Building from source

Install Git, follow the
[flatpak-builder setup guide](https://docs.flatpak.org/en/latest/first-build.html)
then enter the following commands in a terminal:

```bash
git clone --recursive https://github.com/flathub/org.godotengine.Godot.git
cd org.godotengine.Godot/
flatpak install --user flathub org.freedesktop.Sdk//22.08 org.freedesktop.Sdk.Extension.dotnet7//22.08 org.freedesktop.Sdk.Extension.openjdk11//22.08 -y
flatpak-builder --force-clean --install --user -y builddir org.godotengine.Godot.yaml
```

If all goes well, the Flatpak will be installed after building. You can then
run it using your desktop environment's application launcher.

You can speed up incremental builds by installing [ccache](https://ccache.dev/)
and specifying `--ccache` in the flatpak-builder command line (before `builddir`).

### Updating

For updating this Flatpak to newer versions of Godot, the source code of Godot for that version must be downloaded and extracted to some location. The path to it must be updated within the `generate_sources.py` script. It is also necessary to run `generate_sources.py` in the following way:

```
python3 generate_sources.py
```

If `csproj` files that aren't included within the script include new packages, they must be added to the script.

### Double Precision

To compile Godot 4 with double precision, `org.godotengine.Godot.yaml` needs to be modified to include the appropriate flags. Under `Build Commands`, change all instances of `precision=single` to `precision=double`.
