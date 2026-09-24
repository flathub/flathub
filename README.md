# ModLoader64 Flatpak Packaging

This directory contains the files required to build, test, and distribute ModLoader64 as a Flathub-compliant Flatpak.

## Structure

- `com.hylianmodding.modloader64.yml`: Main Flatpak builder manifest.
- `node-sources/`: Offline npm/yarn dependency manifests generated via `flatpak-node-generator`.
- `shared-modules/`: `flathub/shared-modules` submodule (glu, glew).

The desktop entry, AppStream metainfo, and `start-modloader64` launcher wrapper are not
stored in this repo; they are fetched at build time (pinned by commit + sha256) from the
upstream https://github.com/hylian-modding/ModLoader64-flatpak repo.

## Generating Offline Node Sources

Flathub builds run with network access disabled during compilation. All npm and yarn dependencies must be vendored offline using `flatpak-node-generator`:

```bash
# Generate ModLoader64 runtime offline sources (yarn.lock)
flatpak-node-generator yarn ../ModLoader64/yarn.lock -o node-sources/modloader64-core.json

# Generate GUI offline sources (package-lock.json)
flatpak-node-generator npm ../ModLoader64-GUI/package-lock.json -o node-sources/modloader64-gui.json
```

## Emulator, Game Cores, and Mods

This Flatpak bundles the single emulator used by ModLoader64: Mupen64Plus, including the native `ml64_emu_addon.node` binding and required Mupen64Plus plugins. Game cores and community mods are separate ModLoader64 content; they are not emulator cores and can be added by future GUI/update logic or by manual testing.

## Building and Installing Locally

```bash
# Clone the flathub/shared-modules submodule (glu, glew) first
git submodule update --init

# Using flatpak-builder directly or via the Flatpak Builder flatpak
flatpak run org.flatpak.Builder --user --install --force-clean build-dir com.hylianmodding.modloader64.yml
```
