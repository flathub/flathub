# io.github.foxstudio201.dinoisekai

Flatpak packaging for **Dino Isekai** — Minecraft Launcher by FoxStudio.

App ID: `io.github.foxstudio201.dinoisekai`

## Build

```bash
flatpak-builder --force-clean build-dir io.github.foxstudio201.dinoisekai.yml --install-deps-from=flathub
```

## Install

```bash
flatpak-builder --user --install --force-clean build-dir io.github.foxstudio201.dinoisekai.yml
flatpak run io.github.foxstudio201.dinoisekai
```

## Generate Sources

```bash
cd dinoisekailauncher
npm install  # ensures package-lock.json matches package.json
flatpak-node-generator npm --electron-node-headers --xdg-layout -o flatpak/generated-sources.json package-lock.json
```

The generated `generated-sources.json` pins the npm registry cache and the Electron
binary required for a fully offline build.