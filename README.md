# Proton GE flatpak

Proton GE is a custom build of GloriousEggroll's custom distribution of Valve'scompatibility layer for running Windows games on Linux through Steam.

This repository contains recipies for building Proton from [source](https://github.com/GloriousEggroll/proton-ge-custom) on top of [freedesktop-sdk](https://gitlab.com/freedesktop-sdk/freedesktop-sdk) in flatpak format, intended for running from the [Steam flatpak](https://github.com/flathub/com.valvesoftware.Steam).

## Installation

This unofficial build isn't supported by GloriousEggroll nor Valve and wasn't tested with all possible games and cases. It can behave differently from upstream builds. Use at your own risk.

First [add](https://flatpak.org/setup) Flathub repository and install Steam from there, if not already. Then run
```
flatpak install com.valvesoftware.Steam.CompatibilityTool.Proton-GE
```

## Running

Launch Steam flatpak and select "Proton GE (flatpak)" compatibility tool from drop-down list.
