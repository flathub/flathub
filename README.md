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

## Notes

At moment the [flatpak-node-generator](https://github.com/flatpak/flatpak-builder-tools/tree/master/node) tool doesn't support package-lock.json v3. A workaround is generate an Antares `package-lock.json` with `npm i --lockfile-version 2 --package-lock-only` command.
