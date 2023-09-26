This is a shared module for libdecoration, used by several applications and utilities to get window decorations on Wayland compositors that might not implement server-side decorations.

This will contain the latest versioned release of libdecor; if newer (devel) versions are desired, please manually add it to your flatpak manifest.

The freedesktop runtime already bundles GTK3, so you don't need to do any more work to get native-looking CSDs on GNOME with libdecor versions newer than 0.1.1.
If it can't load the GTK3 plugin, it will fallback to Cairo.
