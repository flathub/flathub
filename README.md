# Antares SQL

## Installation

[**Set up Flatpak**](https://www.flatpak.org/setup/)

**Install Antares SQL from cli:**

```shell
flatpak install -y it.fabiodistasio.AntaresSQL
```

**Run Antares:**

```shell
flatpak run it.fabiodistasio.AntaresSQL
```

**To uninstall:**

```shell
flatpak uninstall -y it.fabiodistasio.AntaresSQL
```

## Build

The `flatpak-builder` package is required.

**Install the SDK:**

```shell
flatpak install org.freedesktop.Platform/x86_64/23.08 org.freedesktop.Sdk/x86_64/23.08
```

**Build Antares SQL:**

```shell
flatpak-builder --user --install --force-clean build it.fabiodistasio.AntaresSQL.yml
```
