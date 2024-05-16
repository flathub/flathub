# Syncthing Tray

Flatpak build for [Syncthing Tray](https://github.com/Martchus/syncthingtray).

## Known flatpak related issues

> NOTE:
>
> Before submit an issue, please check whether it is a flatpak package issue or upstream Syncthing Tray issue.

### Autostart not working

Syncthing Tray's autostart is implemented by install a .desktop file in `~/.config/autostart`. However, due to sandboxing by flatpak, this file is placed in `~/.var/app/org.martchus.syncthingtray/config/autostart`. Also the generated `.desktop` is not suitable for flatpak, it actually cannot start the application.

The workaround is to create a symlink manually:

1. Create autostart folder if it does not exist:

```bash
mkdir -p $HOME/.config/autostart
```

2. If installed as per-user application (`flatpak install --user`):

```bash
ln -sf $HOME/.local/share/flatpak/app/org.martchus.syncthingtray/current/active/export/share/applications/org.martchus.syncthingtray.desktop $HOME/.config/autostart/
```

3. If installed as system-wide application (`flatpak install`):

```bash
ln -sf /var/lib/flatpak/app/org.martchus.syncthingtray/current/active/export/share/applications/org.martchus.syncthingtray.desktop $HOME/.config/autostart/
```

To stop from autostart, just remove the symlink:

```bash
rm $HOME/.config/autostart/org.martchus.syncthingtray.desktop
```

### Syncthing Tray stuck or failed on initialize

It is likely that you have previously installed syncthing or Syncthing Tray and leave the configuration file in place.

Try to rename (or remove) these files and run the application again:

```bash
mv ~/.config/syncthingtray.ini ~/.config/syncthingtray.ini.bak
mv ~/.local/state/syncthing ~/.local/state/syncthing.bak
```

### Cannot synchronize files outside of `/home`

The filesystem access is limited to `/home`. To modify it, use [Flatseal](https://flathub.org/apps/com.github.tchx84.Flatseal), or with command:

```bash
flatpak override org.martchus.syncthingtray --filesystem=<PATH>
```

## FAQ

### Should I install syncthing alone?

No you don't. This package already contains the syncthing program.

### Where is the configuration file?

They are located in `~/.var/app/org.martchus.syncthingtray`.

## Deploying

### Generate cpan-generated-sources.json

```bash
chmod +x ./tools/flatpak_cpan_generator.sh
./tools/flatpak_cpan_generator.sh
```

### Build the application

```bash
flatpak run org.flatpak.Builder build-dir org.martchus.syncthingtray.yml
```

### Test the build

```bash
flatpak run org.flatpak.Builder --user --install --force-clean build-dir org.martchus.syncthingtray.yml
flatpak run org.martchus.syncthingtray.yml
```
