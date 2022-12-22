# ArmCord Flatpak

This is the flatpak for ArmCord. Due to sandbox limitations, it might not function as expected, so some ways to get it working are detailed below:

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
TAKEN FROM https://github.com/flathub/com.discordapp.Discord/wiki/Rich-Precense-(discord-rpc)

Flatpak applications need certain changes inside of the flatpak environment to connect properly:

1. Permission to access `$XDG_RUNTIME_DIR/app/xyz.armcord.ArmCord/`
2. A symlink at `$XDG_RUNTIME_DIR/discord-ipc-0` pointing to `$XDG_RUNTIME_DIR/.flatpak/xyz.armcord.ArmCord/xdg-run/discord-ipc-0`

Ask the applications maintainers to do this, don't do it yourself to ensure you continue to get updates.

Suggested changes to accomplish these needs :

1. Add `--filesystem=xdg-run/app/com.xyz.armcord.ArmCord:create` to finish-args
2. Create a wrapper (if you don't already have one) for the apps binary and add something like this:
```sh
    for i in {0..9}; do
        test -S $XDG_RUNTIME_DIR/discord-ipc-$i || ln -sf {app/com.discordapp.Discord,$XDG_RUNTIME_DIR}/discord-ipc-$i;
    done
```
