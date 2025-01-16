# OCRmyPDF Flatpak Maintenance Documentation

## TODO metainfo.xml

Will be part of upstream in the next tag (probably 16.9.0), replace file with `src/ocrympdf/data/io.ocrmypdf.ocrmypdf.xml` in manifest and remove the file from the GitHub repository.

## flatpak-external-data-checker and updates

Checks every module for updates, but will only raise a PR when `ocrmypdf` gets an update. This PR will include all updates available at this point (`flathub.json`).

`pngquant` is built using rust and will require new, manually generated `cargo-sources-pngquant.json` when updated.

`cryptography`(dep of `pdfminer.six`) installs arch-dependend wheels and has to be updated manually.

## pikepdf

Utilizes a submodule for the pikepdf manifest. `dependabot` is configured to check for updates weekly (`.github/dependabot.yml`).

## python modules

Everything was generated with `flapak-pip-generator` and is organized in this structure to make maintenance easy. All python modules (beside `cryptography`) use `noarch` wheels (where available) or are built from source.

`python3-sphinx`is required to build documentation of `pngquant`

`python3-requirements.json` includes the following packages:

```
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

`cryptography`(dep of `pdfminer.six`) uses arch-dependend wheels. Building from source requires manual work because of cargo sources, and the python dependencies `setuptools-rust` and `maturin` (also built with cargo). This might be changed to source builds later, but it is probably not worth it.

`pygments`has to be added manually and with `--ignore-installed` flag, because it is part of the Sdk (but not of the runtime) and gets skipped otherwise.
