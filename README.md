# Flatpak packaging for Endless Key

This package contains [Kolibri](https://learningequality.org/kolibri/) as well
as a GNOME front-end with [Endless Key](https://www.endlessos.org/key) scheme.

## Building

To build and install this package on your system, use flatpak-builder:

    flatpak-builder build-dir org.endlessos.Key.yaml --install --user

Once it is installed, you can run Endless Key:

    flatpak run org.endlessos.Key

Note that the Endless Key flatpak will use a different data directory than if it was
running on the host system. Instead of being located in ~/.kolibri, Kolibri's
database files will be stored in the Flatpak application's data directory, such
as `~/.var/app/org.endlessos.Key/data/kolibri`. This can be changed
as usual by setting the `KOLIBRI_HOME` environment variable.

## Running Kolibri management commands

You can use the `kolibri` command inside the flatpak to run management commands. For example:

    flatpak run --command=kolibri org.endlessos.Key manage listchannels

For information about Kolibri's management commands, please see <https://kolibri.readthedocs.io/en/latest/manage/command_line.html>.

## Adding desktop launchers for Kolibri channels

Kolibri in Endless Key generates desktop launchers as convenient shortcuts to specific channels you have installed. If you would like for these launchers to appear on your desktop, add `$HOME/.var/app/org.endlessos.Key/data/kolibri/content/xdg/share` to your `XDG_DATA_DIRS`.

To achieve this for all users, create a file named `/usr/lib/systemd/user-environment-generators/61-endless-key-desktop-xdg-plugin` with the following contents, then log out and log in again:

    #!/bin/bash
    XDG_DATA_DIRS="$HOME/.var/app/org.endlessos.Key/data/kolibri/content/xdg/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"
    echo "XDG_DATA_DIRS=$XDG_DATA_DIRS"
