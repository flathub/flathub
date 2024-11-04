# Project Rubi-Ka Flatpak

TODO:

- Test on more hardware other than nvidia gpus
- Add CI
- Publish to flathub so people can install with `flatpak install`
- Add GPG key signing to flatpak build process
- troubleshoot some valve pressure vessel bugs with detecting certain hardware libs like intel iris

# Build

Requirements:

- Have `flatpak` installed
- Have `flathub` installed and configured [Flathub Install Guide](https://flathub.org/setup)

```bash
./build.sh
```

The build script will install `org.flatpak.Builder` for you, which is used to run `flatpak-builder` to build this flatpak.

# Debug

```bash
./debug.sh
```


# Run

a) the icon should pop up in your gnome/kde search automatically

b)

```bash
flatpak run com.projectrk.launcher
```

# Notes

- Running `flatpak run com.projectrk.launcher` from commandline will mount `data/project-rk` and create the directory automatically based on whatever path you're currently on in your shell, so if you're in `$HOME` for example, it will build a new prefix in `$HOME/data/project-rk`.
  - If you don't want this behavior on command line, you can use something like Flatseal to remove `--persist=.` from the flatpak config, or use `flatpak override` commands on CLI to do the same. It will use the default flatpak XDG data dir when this happens.
  - I don't know if the above change (removing persist=.) negatively impacts how umu/pressure-vessel behaves. I think there's a lot of compatibility stuff in there that mounts your host's $HOME/.Steam and $HOME/.local dirs inside the flatpak container to expose it to pressure-vessel at runtime so proton can play nice with some calls. It also means you won't be able to use previously installed copies of Proton-GE that live in the `.Steam/compatibilitytools.d` directory.
  - In the future, we could change the `--env=WINEPREFIX=xx` settings in the flatpak build manifest to use explicit paths such as `$HOME/.var/apps/${FLATPAK_ID}/data/project-rk` to avoid this behavior difference between shortcut-launched and shell-launched altogether.
- Running the flatpak from GNOME using the desktop shortcut, for example, means the flatpak is running from the `$HOME/.var/apps/com.projectrk.launcher` and the default WINEPREFIX that will generate and manage will be in `$HOME/.var/apps/com.projectrk.launcher/data/project-rk`. This is the intended way to use this.

# Tracking list

- https://github.com/KhronosGroup/Vulkan-Loader/blob/main/docs/LoaderLayerInterface.md#linux-layer-discovery for pressure-vessel behavior with certain vulkan libs when in a flatpak container
  - https://github.com/flathub/com.valvesoftware.Steam/commit/0538256facdb0837c33232bc65a9195a8a5bc750 notes on the above issue, which is currently a workaround
- https://github.com/ValveSoftware/steam-runtime/issues/474 for weird GL driver missing links between pressure vessel sub container and flatpak container 
