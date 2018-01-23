# Flatpak for [D-Feet](https://wiki.gnome.org/Apps/DFeet)

D-Feet is an easy to use D-Bus debugger. D-Feet can be used to inspect D-Bus interfaces of running programs and invoke methods on those interfaces.

## Notes

* `0001-Rename-the-icons-appdata-and-desktop-files.patch`,
  `0003-build-Install-appstream-metadata-to-non-deprecated-l.patch`: already
  merged upstream. We use `"rm-configure": true` in the manifest to cause
  `autogen.sh` to be re-run.

## Credits

Derived from the [upstream manifest for D-Feet master](https://git.gnome.org/browse/d-feet/tree/org.gnome.d-feet.json), written by Mathieu Bridon.
