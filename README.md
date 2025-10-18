# NoRisk Client Launcher

Welcome to the Flathub repository for **NoRisk Client Launcher**. This repository contains the Flatpak packaging files for distributing the NoRisk Client Launcher via Flathub.

## Install Instructions

Install from flathub (once published):

```bash
flatpak install flathub gg.norisk.NoRiskClientLauncherV3
```

## Build & Install Instructions

To build and install the Flatpak package locally:

```bash
flatpak install -y flathub org.flatpak.Builder
flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak run --command=flathub-build org.flatpak.Builder --install gg.norisk.NoRiskClientLauncherV3.yml
```

## Linting

To lint the manifest and check for common issues:

```bash
flatpak install -y flathub org.flatpak.Builder
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest gg.norisk.NoRiskClientLauncherV3.yml
flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo
```

## Creating a Package

To create a Flatpak bundle for distribution:

```bash
flatpak install -y flathub org.flatpak.Builder
flatpak run --command=flathub-build org.flatpak.Builder gg.norisk.NoRiskClientLauncherV3.yml
flatpak build-bundle repo norisk-client-launcher.flatpak gg.norisk.NoRiskClientLauncherV3 --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo
```

## Maintainers

-   Greenman999 (@Greeenman999)

## License

See [LICENSE](./LICENSE) for details.
