# Scratch Flatpak

## Re-generating the sources

- Run `update-package-info.py` to read the scratch-desktop Git tag from the manifest and generate
  the latest asset sources & download the needed package\*.json files.
- Run `npm install` inside of `scratch-desktop/node_modules/scratch-gui` (which is written by
  update-package-info.py) to create the needed package-lock.json.
- Run:

    flatpak-node-generator.py npm scratch-desktop/package-lock.json -P -R scratch-\*/package-lock.json -s

  afterwards to regenerate the npm package sources.
