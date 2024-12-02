This repository contains commonly shared modules and is intended to be used as a git submodule.

Each submodule may include additional instructions to be used properly. Please check the folder containing that module to see if anything extra needs to be done.

## Adding

To use shared modules for packaging an application, add the submodule:

```
git submodule add https://github.com/flathub/shared-modules.git
```

## Usage

Then modules from this repository can be specified in an application
manifest.

```json
"modules": [
  "shared-modules/SDL/SDL-1.2.15.json",
  {
    "name": "foo"
  }
]
```
And for a YAML manifest:
```YAML
modules:
  - shared-modules/SDL/SDL-1.2.15.json

  - name: foo
```

## Updating

To update the submodule:

```
git submodule update --remote --merge
```

## Removing

To remove the submodule:

```
git submodule deinit -f -- shared-modules
rm -rf .git/modules/shared-modules
git rm -f shared-modules
rm .gitmodules
```

We provide an automatic updating mechanism for submodules located here. In order to utilize it:

- Set up x-checker-data for your sources: https://github.com/flathub/flatpak-external-data-checker#url-checker

And it will check for updates in a weekly basis.

Please do not request adding modules unless they are widely used in
the Flathub repository.

All shared modules manifests in this repository are, and need to be, in the JSON format,
which is supported by both Flatpak manifest formats, JSON and YAML.
