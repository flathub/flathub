# org.artisan_scope.artisan

Flatpak for [Artisan](http://artisan-scope.org/).

## Updating Python dependencies

The build process does not have network access (by design). All files required need to be declared, and are downloaded by the builder.
For the Python dependencies, there are many files. After an application update, the list can be re-generated with the help of
[`req2flatpak`](https://github.com/johannesjh/req2flatpak) and [`flatpak-pip-generator`](https://github.com/flatpak/flatpak-builder-tools/tree/master/pip).
[PyQt Flatpak documentation](https://develop.kde.org/docs/getting-started/python/pyqt-flatpak/) suggests `flatpak-pip-generator`,
but building SciPy from source did not pass its tests, while `req2flatpak`'s use of binary wheels do. Only for matplotlib and
Pillow, we build from source, so that the existing libraries can be used (instead of a bundled copy).

Some tweaks are needed, however, because we don't use `pyinstaller`, and qt-related things are already
present in the BaseApp.

Make sure you have the necessary tools installed:

```
python3 -m pip install req2flatpak pip-tools
wget https://raw.githubusercontent.com/flatpak/flatpak-builder-tools/master/pip/flatpak-pip-generator
```

Then copy the file `src/requirements.txt` from the version of Artisan you're updating to, to the current directory
(e.g. with the help of `tar -x -z --strip-components=2 --wildcards -f vX.Y.Z.tar.gz \*/src/requirements.txt`),
then run `update-dependencies.sh`.

