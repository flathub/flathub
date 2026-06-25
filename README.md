# Legcord Flatpak

<!-- This flatpak is based on dev.vencord.Vesktop @ https://github.com/flathub/dev.vencord.Vesktop, which itself is based on our old flatpak on xyz.armcord.ArmCord @ https://github.com/flathub/xyz.armcord.ArmCord . Funny how that works! -->

This is the flatpak for [Legcord](https://github.com/Legcord/Legcord).

## Wayland

Legcord will run through Wayland by default with fallback to x11, as this is the most compatible option.
Everything should work out of the box, including screen sharing and hardware acceleration.

If you wish to run it through XWayland on Wayland instead, you can do so by adding the `--socket=x11` permission and removing the `--socket=wayland` and `--socket=x11-fallback` permissions with [Flatseal](https://flathub.org/apps/com.github.tchx84.Flatseal) or by running the following commands:

```sh
flatpak override --nosocket=wayland app.legcord.Legcord
flatpak override --nosocket=x11-fallback app.legcord.Legcord
flatpak override --socket=x11 app.legcord.Legcord
```

## File access

Due to the Flatpak sandbox, Legcord only has access to a very limited set of files, which messes with file Drag & Drop and Copy Paste.

As a workaround, you can either use solely the built-in file picker, or you can give Legcord
access to your home directory (& other desired directories) using [Flatseal](https://flathub.org/apps/com.github.tchx84.Flatseal) or by running the following command:

```sh
flatpak override --filesystem=home app.legcord.Legcord
```

## Tray icons

To get a working Tray Icon on GNOME, install the [appindicator-support](https://extensions.gnome.org/extension/615/appindicator-support/) extension.

## Discord Rich Presence

Game Activity on the flatpak is very limited, as the sandbox does not allow Legcord to scan running processes.
This means that Rich Presence will only work for games that explicitly support it.

Follow the instructions below to enable Rich Presence for such applications.

### Native applications
A solution that works short-term is to run `ln -sf $XDG_RUNTIME_DIR/{.flatpak/app.legcord.Legcord/xdg-run,}/discord-ipc-0`.
For something longer lasting, run the following:

```sh
mkdir -p ~/.config/user-tmpfiles.d
echo 'L %t/discord-ipc-0 - - - - .flatpak/app.legcord.Legcord/xdg-run/discord-ipc-0' > ~/.config/user-tmpfiles.d/discord-rpc.conf
systemctl --user enable --now systemd-tmpfiles-setup.service
```
Now, native applications will be able to use Rich Presence on every system start.

### Flatpak applications
<!-- TAKEN FROM https://github.com/flathub/com.discordapp.Discord/wiki/Rich-Precense-(discord-rpc) -->

Flatpak applications need certain changes inside of the flatpak environment to connect properly:

1. Permission to access `$XDG_RUNTIME_DIR/.flatpak/app.legcord.Legcord/`
2. A symlink at `$XDG_RUNTIME_DIR/discord-ipc-0` pointing to `$XDG_RUNTIME_DIR/.flatpak/app.legcord.Legcord/xdg-run/discord-ipc-0`

Suggested changes to accomplish these needs :

1. Add `--filesystem=xdg-run/.flatpak/app.legcord.Legcord:create` and `--filesystem=xdg-run/discord-ipc-0` to the global Flatpak permissions
2. Restart
