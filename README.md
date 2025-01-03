# org.freedesktop.Sdk.Extension.tup

Provides the [Tup build system](https://gittup.org/tup) for use during the build process of a Flatpak package.

## Usage

Add the following to your manifest file:

```yaml
sdk-extensions:
  - org.freedesktop.Sdk.Extension.tup
build-options:
  append-path: /usr/lib/sdk/tup/bin
```

This enables the `tup` binary for the `build-commands` of the manifest's modules.
