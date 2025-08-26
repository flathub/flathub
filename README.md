# Feishin Flatpak

Flatpak for the [Feishin](https://github.com/jeffvli/feishin) music player for Navidrome and Jellyfin.
Contains mpv and a default config file so the only setup needed should be to connect to your media server.

# Wayland Support

Simply remove the X11 permission, and add the Wayland permission to the flatpak.
Then edit the application entry and add `--enable-features=UseOzonePlatform,WaylandWindowDecorations --ozone-platform-hint=auto`
to the end of the launch arguments.
