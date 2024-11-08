![image](metadata/icons/icon128.png)

# Project Rubi-Ka

## Build

### Requirements:

- Have `flatpak` installed

```bash
./build.sh
```

> [!NOTE]
> We have to pre-install supplementary 32-Bit drivers beforehand since they're not listed as dependencies of the org.freedesktop.Platform runtimes, they're treated as optional application dependencies- so when the flatpak gets installed from the local `repo/` directory that gets created from build output, there's no available refs that flatpak can auto-resolve and install from (GL32 and i386 extensions will not exist in `repo/`, they exist in `flathub`)




## Debug

- adds --devel flag to the run command and overrides the flatpak/container entry point with `bash` to get you into an interactive shell.

```bash
./debug.sh
```



## Run from CLI

```bash
flatpak run com.projectrk.launcher
```

### Environment Variable Options (tweaking/customization)

#### dgVoodooCpl.exe

> [!TIP]
> This flatpak contains an optional/additional shortcut and entrypoint that invokes dgVoodooCpl.exe from within the client application folder, which is used to configure dgVoodoo2. dgVoodoo is used to translate the game's original DX7 graphics calls to DX11 and DX12. For some people, depending on their hardware, they may want to downgrade their graphics API version.
> More about dgVoodoo2 can be read [here](https://dege.freeweb.hu/dgVoodoo2/). 

```bash
flatpak run --command=dgVoodooCpl com.projectrk.launcher
```

This flatpak also has a desktop shortcut for dgVoodooCpl.exe. ("Project RK (DGV)")

#### Wine/Proton

- Native will use the prefix's local binaries.
- Builtin will use the binaries built into WINE.
```bash
flatpak run --env=WINEDLLOVERRIDES='DDraw.dll,D3DImm.dll=n,b' # overrides these dlls and sets them to priority Native>Builtin. 
```

#### UMU Launcher

- See [UMU Documentation](https://github.com/Open-Wine-Components/umu-launcher/blob/main/docs/umu.1.scd) for more details
```bash
# GE or UMU will change the version of proton that gets installed dynamically by umu-run, the launcher
flatpak run --env=PROTONPATH=UMU-Proton com.projectrk.launcher

# setting this number/ID will make umu automatically apply ecosystem-managed protonfixes to your prefix.
flatpak run --env=GAMEID=x com.projectrk.launcher
```

#### Steam Linux Runtime (Pressure Vessel)
```bash
# For advanced users
# Will get you into the steam linux runtime sub-container xterm based terminal emu+shell
flatpak run --env=PRESSURE_VESSEL_SHELL=instead com.projectrk.launcher
```
> [!IMPORTANT] If you use PRESSURE_VESSEL_SHELL=instead, and the terminal is really hard to see, hold CTRL+right mouse click to see the xterm context menu, then enable the fonts supported by your host.
