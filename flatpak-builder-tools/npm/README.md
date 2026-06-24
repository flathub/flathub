> [!WARNING]
> This is deprecated use [flatpak-node-generator](../node/README.md).

This is a tool to take npm5 `package-lock.json` lock files and generate flatpak-builder
sources for this that let you build the npm-using app in flatpak-builder without
network access.

When you run the tool on the `package-lock.json` file from the repo,
outside the sandbox it will generate a json file with extra sources
that will let `npm install --offline` work.

The included `io.atom.electron.ElectronQuickStart.json` sample shows how to use this
and can be built with:

	./flatpak-npm-generator.py electron-quick-start-package-lock.json -o generated-sources.json
	flatpak-builder --force-clean --repo=repo app io.atom.electron.ElectronQuickStart.json

Note that this requires `io.atom.electron.BaseApp` installed. It is available on flathub.

Also, this repo contains a file `electron-quick-start-package-lock.json` which was
created by running `npm install` in the electron-quick-start. Normally however, such
files would be checked in to git.
