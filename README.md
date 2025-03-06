# Flatpak for [Ausweiskopie](https://github.com/varbin/ausweiskopie)

This is the repository to build a flatpak for _Meine Ausweiskopie_, a tool to create redacted and watermarked copies of German identity documents (passport and identity cards).

## Notes for maintainers

### Build Process

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
 - Currently (2025-03-06) Tk/Tcl 9.0 is not installed, therefore Tcl/Tk 8.6 is used.
 - meson and setuptools must be manually added to the respective file, as flatpak-pip-generator will ignore them
 - python-dbus needs special care: It needs a dbus binary, meson, and environment variables to link to libm...

### Updating

Most sources are to be compatible with

Rerun the flatpak-pip-generator on the requirements.txt:
```
flatpak-pip-generate -r requirements.txt
```
