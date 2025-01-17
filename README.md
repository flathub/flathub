# OCRmyPDF Flatpak Maintenance Documentation

## TODO metainfo.xml

Will be part of upstream in the next tag (probably 16.9.0), replace file path in install command with `misc/flatpak/io.ocrmypdf.ocrmypdf.xml` in manifest and remove the file download from the manifest.

## flatpak-external-data-checker and updates

Checks every module for updates, but will only raise a PR when `ocrmypdf` gets an update. This PR will include all updates available at this point (`flathub.json`).

`pngquant` is built using rust and will require new, manually generated `cargo-sources-pngquant.json` when updated.

`cryptography`(dep of `pdfminer.six`) installs arch-dependend wheels and has to be updated manually.

## python modules

`python3-sphinx`is required to build documentation of `pngquant`

`python3-requirements.json` includes the following packages in this order:

```
lxml
pybind11
pikepdf (requires qpdf)
deprecation
img2pdf
packaging
pdfminer.six
pi-heif
Pillow
pluggy
rich
hatch-vcs
pygments
```

`cryptography`(dep of `pdfminer.six`) uses arch-dependend wheels and requires manual updates.

`pygments`has to be added manually and with `--ignore-installed` flag, because it is part of the Sdk (but not of the runtime) and gets skipped otherwise.
