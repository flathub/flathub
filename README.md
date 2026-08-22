# Justice Client Flatpak

Flatpak packaging for the Justice Client Minecraft launcher
(`org.justiceclient.launcher`). The launcher ships prebuilt (a `.tar.gz`
payload built by `build-appimage.sh`), which this manifest downloads via
`extra-data` and unpacks at install time  no source code involved.

**Status:** built and tested installs and launches on GNOME 50 runtime.

## Build locally

```sh
sudo pacman -S flatpak flatpak-builder   # one-time, needs root
./build-flatpak.sh                        # builds + installs for your user
./build-flatpak.sh bundle                 # ...also exports a .flatpak bundle
flatpak run org.justiceclient.launcher    # run it
```

## Publish on Flathub

1. Push this repo (manifest + desktop/metainfo/icon/apply_extra) to a GitHub
   repo named `org.justiceclient.launcher`
2. Fork https://github.com/flathub/flathub, add it as a submodule, and open a
   PR:
   ```sh
   git submodule add https://github.com/kapaisu/org.justiceclient.launcher.git
   ```
3. Flathub CI builds and publishes it; users then:
   ```sh
   flatpak install flathub org.justiceclient.launcher
   ```

## Files

- `org.justiceclient.launcher.yml`  the Flatpak manifest (GNOME 50 runtime)
- `org.justiceclient.launcher.desktop`  menu entry (also registers the
  `justice://` scheme and `.mrpack` file association)
- `org.justiceclient.launcher.metainfo.xml`  AppStream metadata (required by Flathub)
- `org.justiceclient.launcher.png`  app icon
- `apply_extra`  unpacks the payload tarball at install time
  (installed to `/app/bin/apply_extra`, which is where flatpak runs it)

## How it works / gotchas solved

- **Payload:** `extra-data` downloads `Justice-Client-<ver>-x86_64.tar.gz`
  from GitHub Releases (built by `build-appimage.sh`). The AppImage itself
  can't be used: flatpak runs `apply_extra` in a sandbox with `/proc` disabled,
  and the AppImage runtime needs `/proc/self/exe` to self-extract. GNU tar
  needs nothing, so a tarball works.
- **apply_extra location:** flatpak wipes and re-creates `/app/extra` at
  deploy time, and runs the unpacker from `/app/bin/apply_extra`  so the
  script must live in `/app/bin`, not `/app/extra`.
- **libbz2 soname:** the binary is built on Arch, whose libbz2 has the SONAME
  `libbz2.so.1.0`; the Debian-based GNOME runtime only ships the
  `libbz2.so.1` alias. A symlink shim in `/app/lib` + `LD_LIBRARY_PATH` fixes it.
- **Sandbox:** network + `~/` access (game state and auto-downloaded Java in
  `~/.justice-launcher`), X11/Wayland + GPU for the 3D skin viewer.

## Updating to a new release

1. `build-appimage.sh` → upload BOTH the `.AppImage` and `.tar.gz` to GitHub Releases
2. Bump `url`/`sha256`/`size` in the `extra-data` source in the manifest
3. Bump `<release>` in the metainfo, commit, push
