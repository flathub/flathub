Steam Link - Flatpak packaging
==============================

Layout
------

`com.valvesoftware.SteamLink.yml` is the top-level manifest file
(install Flatpak and look at flatpak-manifest(5)).

At the moment, it builds:

* a specially patched version of `qtbase` with more controller support
    * `patches/steamlink/qt-everywhere-src-5.14.1.patch` is from the
        Steam Link SDK and is not used directly
    * `patches/steamlink/qtbase.patch` is a version of that patch that
        has been put through `filterdiff` (see the comment at the top)
        to only include the `qtbase` parts
    * `patches/org.kde.Sdk/` are Flatpak integration improvements taken from
        https://gitlab.com/freedesktop-sdk/flatpak-kde-runtime/-/tree/qt5.14/
    * TODO: Can we update this version of Qt to something more modern?
    * TODO: Has some/all of the controller support gone upstream?
* `qtsvg`
    * this is straight from org.kde.Sdk, and not patched
* `chrpath`
    * we use this to remove unwanted RPATHs
    * Debian is the closest it has to an upstream developer

and also pulls together:

* precompiled dependencies
    * suitable snapshots of SDL
    * a suitable snapshot of FFmpeg
* the Steam Link main executable
* icons
* [AppStream](https://www.freedesktop.org/software/appstream/docs/) metadata
* a [Desktop Entry](https://specifications.freedesktop.org/desktop-entry-spec/latest/)
    to launch it with

Rebuilding this package locally
-------------------------------

### Install dependencies on build machine

* `apt install flatpak-builder` or your distro's equivalent
* `flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo`
    * Add `--user` if you prefer to install into
        `~/.local/share/flatpak` instead of `/var/lib/flatpak`
* `flatpak install org.freedesktop.Sdk/x86_64/20.08`

### Get the binaries

The Steam Link binaries aren't available to the public yet, so the
manifest can't refer to them by URL like it can when they're public.
Instead, for now:

* Look in `com.valvesoftware.SteamLink.yml` to find what binaries are
    expected to be found in `_source/`
* Create a `./_source/` directory
* Put the archive(s) in there

### Build

Run `flatpak-builder(1)` according to its documentation. For convenience,
`./build.sh` has a suitable command which will do the build in `./_build`.
flatpak-builder will maintain a cache in `./.flatpak-builder` so that it
doesn't have to rebuild Qt every time.

To deploy the resulting app on a test machine, the easiest way is to
tell Flatpak to build a standalone "bundle" (a `.flatpak` file).
Again, `./build.sh` has a suitable command.

### Install dependencies on test machine

* `apt install flatpak` or your distro's equivalent
* Log out and back in, if you didn't already have Flatpak installed
    (this makes sure your desktop environment will load the files
    "exported" by Flatpak apps, like their `.desktop` files)
* `flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo`
    * Add `--user` if you prefer to install into
        `~/.local/share/flatpak` instead of `/var/lib/flatpak`
* `flatpak install org.freedesktop.Platform/x86_64/20.08`

### Install on test machine

* Copy `_build/steamlink.flatpak` to the test machine
* `flatpak install /path/to/steamlink.flatpak`
    * Add `--user` if you prefer to install into
        `~/.local/share/flatpak` instead of `/var/lib/flatpak`
