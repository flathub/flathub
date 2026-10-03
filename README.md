# LiquidLauncher

Flatpak packaging for [LiquidLauncher](https://github.com/CCBlueX/LiquidLauncher), a custom Minecraft launcher for LiquidBounce.

## Build

```bash
flatpak install -y flathub org.flatpak.Builder
flatpak run --command=flathub-build org.flatpak.Builder --install net.ccbluex.liquidlauncher.yml
```

## Lint

```bash
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest net.ccbluex.liquidlauncher.yml
flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo
```

`cargo-sources.json` and `node-sources.json` are generated from the release's `Cargo.lock` and `yarn.lock` by the update workflow.
