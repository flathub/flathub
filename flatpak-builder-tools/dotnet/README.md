# Flatpak .NET Generator

Tool to automatically generate a `flatpak-builder` sources file from a .NET Core .csproj file.

## Requirements

You need to have `org.freedesktop.Sdk` and `org.freedesktop.Sdk.Extension.dotnet` installed,
both branch 18.08.

## Usage

Run `flatpak-dotnet-generator.py my-output-sources.json my.input.Desktop.csproj`. Then,
you can use the sources file like this:

```yaml
modules:
  - name: my-module
    buildsystem: simple
    build-commands:
      - '. /usr/lib/sdk/dotnet/enable.sh; dotnet build -f netcoreapp2.1 -c Release --source nuget-sources my.input.Desktop.csproj'
    sources:
      - my-output-sources.json
```

As you can see, the sources file generated will have flatpak-builder save everything into
the *nuget-sources* directory. If you want to change the directory name, run
`flatpak-dotnet-generator.py` with `--destdir=my-destdir`.
