# org.artisan_scope.artisan

Flatpak for [Artisan](http://artisan-scope.org/).

## Updating Python dependencies

The build process does not have network access (by design). All files required need to be declared, and are downloaded by the builder.
For the Python dependencies, there are many files. After an application update, the list can be re-generated with the help of
[flatpak-pip-generator](https://github.com/flatpak/flatpak-builder-tools/tree/master/pip)
as described in the [PyQt Flatpak documentation](https://develop.kde.org/docs/getting-started/python/pyqt-flatpak/).

Some tweaks are needed, however, because the tool doesn't handle dependencies with platform and version specifiers correctly
([flatpak-builder-tools#365](https://github.com/flatpak/flatpak-builder-tools/issues/365)). This can be worked around by
adapting `requirements.txt`.

First get the file `src/requirements.txt` from the version of Artisan you're updating to. Then run:

```
# make sure you have the new version of Artisan's requirements.txt in the directory you're in
cat >requirements-filtered.txt <<EOF
# Numpy + Scipy requirements for building from source with older Cython in BaseApp
meson-python
pybind11
numpy==1.26.4
pythran
scipy==1.10.1
# matplotlib build dependency
cppy
# aiohttp build dependency
expandvars
EOF
cat requirements.txt | \
  grep -v '\(^PyQt\|^qt[0-9]\+-tools\|^scipy\s*[=<>]\|^numpy\s*[=<>]\|^pydantic\|^pyinstaller\)' | \
  grep -v "\\(python_version\s*<\\|;\\s*sys_platform\\s*==\\s*'darwin'\\|;\\s*platform_system\\s*==\\s*'Windows'\\)" \
    >>requirements-filtered.txt
```

Then run `flatpak-pip-generator` with the requirements file:

```sh
wget -nc https://raw.githubusercontent.com/flatpak/flatpak-builder-tools/master/pip/flatpak-pip-generator
BASEAPP_ID=`cat org.artisan_scope.artisan.yml | sed 's/^base:\s*//p;d'`
BASEAPP_VER=`cat org.artisan_scope.artisan.yml | sed 's/^base-version:\s*//p;d' | sed "s/'//g"`
python3 flatpak-pip-generator --runtime "${BASEAPP_ID}//${BASEAPP_VER}" -r requirements-filtered.txt -o dep-python3-artisan
```

Finally perform following tweaks in `dep-python3-artisan.json`:
- Add `--config-settings=setup-args=\"-Dsystem-freetype=true\" --config-settings=setup-args=\"-Dsystem-qhull=true\"` to the `pip` install line for `matplotlib` (see [matplotlib note](https://matplotlib.org/stable/install/dependencies.html#use-system-libraries)).

