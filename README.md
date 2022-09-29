# Radare Iaito for Flatpak

Official Radare Iaito Flatpak package.

<!--
<a href='https://flathub.org/apps/details/org.radare.iaito'><img width='120' alt='Download on Flathub' src='https://flathub.org/assets/badges/flathub-badge-en.png'/></a>
-->

## Permissions

- GUI: x11, ipc, wayland, fallback-x11, dri.
- Radare ptrace: allow devel.

## Special configurations

Since the application folder is readonly it has been enabled the following configurations and paths have been changed:

- Radare 2 configuration: `~/.var/app/org.radare.iaito/config/radare2/`
- Radare 2 data: `~/.var/app/org.radare.iaito/data/radare2/`
