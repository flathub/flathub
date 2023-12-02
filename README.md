# ArmCord Flatpak

This is the flatpak for ArmCord. Due to sandbox limitations, it might not function as expected, so some ways to get it working are detailed below:

## Tray icons

This flatpak has the appropriate permissions for tray icons out of the box; however, GNOME does not provide native tray icons support out of the box, due to the current specification being *horribly* outdated and not being sandbox-friendly.

The extension that should be used to obtain tray icons is [appindicator-support](https://extensions.gnome.org/extension/615/appindicator-support/). Enable this extension and disable any other alternatives and tray icons will function as expected.

## Discord Rich Presence
### Native applications
A solution that works short-term is to run `ln -sf $XDG_RUNTIME_DIR/{.flatpak/xyz.armcord.ArmCord/xdg-run,}/discord-ipc-0`. 
For something longer lasting, run the following:

```sh
mkdir -p ~/.config/user-tmpfiles.d
echo 'L %t/discord-ipc-0 - - - - .flatpak/xyz.armcord.ArmCord/xdg-run/discord-ipc-0' > ~/.config/user-tmpfiles.d/discord-rpc.conf
systemctl --user enable --now systemd-tmpfiles-setup.service
```
Now, native applications will be able to use Rich Presence on every system start.

### Flatpak applications
<!-- TAKEN FROM https://github.com/flathub/com.discordapp.Discord/wiki/Rich-Precense-(discord-rpc) -->

Flatpak applications need certain changes inside of the flatpak environment to connect properly:

1. Permission to access `$XDG_RUNTIME_DIR/.flatpak/xyz.armcord.ArmCord/`
2. A symlink at `$XDG_RUNTIME_DIR/discord-ipc-0` pointing to `$XDG_RUNTIME_DIR/.flatpak/xyz.armcord.ArmCord/xdg-run/discord-ipc-0`

Suggested changes to accomplish these needs :

1. Add `--filesystem=xdg-run/.flatpak/com.xyz.armcord.ArmCord:create` and `--filesystem=xdg-run/discord-ipc-*` to the global Flatpak permissions
2. Restart
