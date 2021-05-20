<img src="https://github.com/vinceliuice/Sierra-gtk-theme/blob/imgs/logo.png" alt="Logo" align="right" /> Mojave-Style Theme for GTK Flatpak Applications
======

This Flatpak provides a copy of [Vince Liuice's Mojave-style GTK theme](https://github.com/vinceliuice/Mojave-gtk-theme) in a form that can be used by Flatpak applications.

> **NOTE:** This theme is a supplement to the main theme and cannot be enabled on its own. In order to apply this theme to your Flatpak applications, you will need to [install the main (non-Flatpak) package from Vince Liuice's GitHub](https://github.com/vinceliuice/Mojave-gtk-theme#Installation) and enable it in [GNOME Tweaks](https://wiki.gnome.org/Apps/Tweaks).

In addition to this Flatpak theme, the following components are currently available:

* [A Snap version of this theme (for Snap applications)](https://snapcraft.io/mojave-themes)
* [A cross-platform installer (for non-sandboxed applications)](https://github.com/vinceliuice/Mojave-gtk-theme)
* [A Mojave-style circular icon theme](https://github.com/vinceliuice/McMojave-kde)
* [A Firefox theme (in the cross-platform installer)](https://github.com/vinceliuice/Mojave-gtk-theme/tree/master/src/other/firefox)
* Themes for both [Dash-to-Dock](https://github.com/vinceliuice/Mojave-gtk-theme/tree/master/src/other/dash-to-dock) and [Plank](https://github.com/vinceliuice/Mojave-gtk-theme/tree/master/src/other/plank)

Please visit Vince Liuice's GitHub for information on the other available packages, as well as the editable source code.

Note that if you install a modified version of the source theme, you can apply it to Flatpak applications, as well, by copying the contents of `~/.themes/<theme_name>/gtk-3.0` to `/var/lib/flatpak/org.gtk.Gtk3theme.<theme_name>/*/*/active/files`. Any updated files you copy will automatically be applied to each Flatpak application the next time you launch it.

## Requirements

Unlike the upstream theme, this package has no external dependencies other than Flatpak itself (and, well, the non-Flatpak version of the theme). However, in order to simplify installation, it only contains the bare minimum of rendered/exported files necessary for applications to use the theme. I may modify this package to include and install the full source from Vince Liuice's repository at some point in the future.

Because of the peculiarities in Flatpak's management of packaged themes, each package can only contain a single variation of the parent theme. For full compatilibility, I recommend installing all available variations of the theme `org.gtk.Gtktheme.Mojave-*`.

## Screenshots

![01](https://github.com/vinceliuice/Mojave-gtk-theme/blob/images/screenshot01.jpeg?raw=true)

![02](https://github.com/vinceliuice/Mojave-gtk-theme/blob/images/screenshot02.jpeg?raw=true)

![03](https://github.com/vinceliuice/Mojave-gtk-theme/blob/images/screenshot03.jpeg?raw=true)

![04](https://github.com/vinceliuice/Mojave-gtk-theme/blob/images/screenshot04.jpeg?raw=true)

![05](https://github.com/vinceliuice/Mojave-gtk-theme/blob/images/screenshot05.jpeg?raw=true)
