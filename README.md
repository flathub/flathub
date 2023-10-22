# Antares SQL

## Installation

[**Set up Flatpak**](https://www.flatpak.org/setup/)

**Install Antares SQL from cli:**

```shell
flatpak install -y app.antares.Antares-SQL
```

**Run Antares:**

```shell
flatpak run app.antares.Antares-SQL
```

**To uninstall:**

```shell
flatpak uninstall -y app.antares.Antares-SQL
```

## Build

The `flatpak-builder` package is required.

**Install the SDK:**

```shell
flatpak install org.freedesktop.Platform/x86_64/23.08 org.freedesktop.Sdk/x86_64/23.08
```

**Build Antares SQL:**

```shell
flatpak-builder --user --install --force-clean build app.antares.Antares-SQL.yml
```
