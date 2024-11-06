# Project Rubi-Ka Flatpak

- Add CI
- Publish to flathub so people can install with `flatpak install`
- Add GPG key signing to flatpak build process

# Build

Requirements:

- Have `flatpak` installed

```bash
./build.sh
```

The build script does the following:

- Installs the `flathub` named flatpak remote for the `user` instead of `system`, since many distros will already have `flathub` pre-configured for `system`.
- Installs org.freedesktop.Platform.Compat.i386//$FDO_VERSION to enable multiarch lib compatibility for 32-bit applications and graphics drivers
- Installs org.freedesktop.Platform.GL32.nvidia-(your kernel module version) if you're on nvidia
- Installs org.freedesktop.Platform.GL32.default//$FDO_VERSION if you're on intel/amd
- Installs the org.flatpak.Builder flatpak to allow you to build the flatpak from the project manifest

We have to pre-install supplementary 32-Bit drivers beforehand since they're not listed as dependencies of the org.freedesktop.Platform runtimes, they're treated as optional application dependencies- so when the flatpak gets installed from the local `repo/` directory that gets created from build output, there's no available refs that flatpak can auto-resolve and install from (GL32 and i386 extensions will not exist in `repo/`, they exist in `flathub`)

When this package is hosted on flathub, dep resolution will function.


# Debug

- adds --devel flag to the run command and overrides the flatpak/container entry point with `bash` to get you into an interactive shell.

```bash
./debug.sh
```


# Run

## From Native Search (GNOME/KDE)

- Search "Project Rubi-Ka"
- Click the application shortcut

## From CLI

```bash
flatpak run com.projectrk.launcher
```

### Environment Variable Options (tweaking/customization)

#### Wine/Proton
```bash
# flatpak run <options> com.projectrk.launcher
--env=WINEDLLOVERRIDES='example.dll,example2.dll=n,b' # overrides these dlls and sets them to priority Native>Builtin. 
```

#### UMU Launcher

- See [UMU Documentation](https://github.com/Open-Wine-Components/umu-launcher/blob/main/docs/umu.1.scd) for more details
```bash
--env=PROTONPATH=xxx-Proton # GE or UMU will change the version of proton that gets installed dynamically by umu-run, the launcher
--env=GAMEID=x # setting this number/ID will make umu automatically apply ecosystem-managed protonfixes to your prefix.
```

#### Steam Linux Runtime (Pressure Vessel)
```bash
--env=PRESSURE_VESSEL_SHELL=instead # super useful, interrupts the game launch and drops you into an interactive xterm window that comes from the nested Steam Linux Runtime container spawned from bwrap, which is 2 layers in. Good if you need to see how the actual linux filesystem looks to the actual proton/game executable.
```

- If you use PRESSURE_VESSEL_SHELL=instead, and the terminal is really hard to see, hold CTRL+right mouse click to see the xterm context menu, then enable the fonts supported by your host.
