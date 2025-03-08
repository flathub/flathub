# Flatpak for [Meine Ausweiskopie](https://github.com/varbin/ausweiskopie)

This is the repository to build a flatpak for _Meine Ausweiskopie_, a tool to create redacted and watermarked copies of German identity documents (passport and identity cards).

## Build Process

The build instruction installs the following packages:
  - freetype (font/text rendering engine)
  - fontconfig (locate fonts)
  - Tcl and Tk
  - Python
  - freedesktop dbus: Build requirement for dbus-python
  - setuptools, meson, meson-python: build dependencies for various Python packages
  - the actual app and its requirements

Idiosyncrasies:
 - A custom Tcl/Tk and therefore Python version are only used because the provided Tk/Tcl is build without proper font support.
 - meson and setuptools must be manually updated in the respective file, as flatpak-pip-generator will ignore them
 - python-dbus needs to be manually updated, too: Besides that it needs a dbus binary, meson, it requires custom environment variables to link to libm...

## Updating

1. Most sources are to be compatible with the [flatpak external data checker](https://github.com/flathub-infra/flatpak-external-data-checker).
2. Rerun the *flatpak-pip-generator* on the requirements.txt: `flatpak-pip-generate -r requirements.txt --ignore-pkg ausweiskopie`
3. Update the tag for the metadata.
