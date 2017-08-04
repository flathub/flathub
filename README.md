# Flatpak manifest for Transmission

Transmission is a BitTorrent client ([website](https://transmissionbt.com/), [Git](https://github.com/transmission/transmission)). This is a [Flatpak](http://flatpak.org/) manifest for its [Gtk+](https://www.gtk.org/) version.

This manifest allows Transmission full access to:

* the network, for obvious reasons
* X11 and Wayland, also for obvious reasons
* the host filesystem, because Transmission hasn't been adapted to use [portals](https://github.com/flatpak/flatpak/wiki/Portals) to open `.torrents` and read/write downloads

The appdata file alongside this manifest has been [accepted upstream](https://github.com/transmission/transmission/pull/224) but is not yet part of a release.

## Colophon

This manifest is derived from that published by Pierre Dureau at <https://github.com/pdureau/flatpak-manifests.git>, via an [intermediate version used for Endless OS](https://github.com/endlessm/transmission-flatpak).
