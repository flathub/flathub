# .NET 9 SDK extension

## How to use
You need to add following lines to flatpak manifest:

```json
"sdk-extensions": [
    "org.freedesktop.Sdk.Extension.dotnet9"
],
"build-options": {
    "append-path": "/usr/lib/sdk/dotnet9/bin",
    "append-ld-library-path": "/usr/lib/sdk/dotnet9/lib",
    "env": {
        "PKG_CONFIG_PATH": "/app/lib/pkgconfig:/app/share/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig:/usr/lib/sdk/dotnet9/lib/pkgconfig"
    }
},
```

###  Scripts
* `install.sh` - copies dotnet runtime to package.
* `install-sdk.sh` - copies dotnet SDK to package.

### Publishing project

```json
"build-commands": [
    "install.sh",
    "dotnet publish -c Release YourProject.csproj",
    "cp -r --remove-destination /run/build/YourProject/bin/Release/net9.0/publish/ /app/bin/",
]
```

### Using nuget packages
If you want to use nuget packages it is recommended to use the [Flatpak .NET Generator](https://github.com/flatpak/flatpak-builder-tools/tree/master/dotnet) tool. It generates sources file that can be included in manifest.

```json
"build-commands": [
    "install.sh",
    "dotnet publish -c Release --source ./nuget-sources YourProject.csproj",
    "cp -r --remove-destination /run/build/YourProject/bin/Release/net9.0/publish/ /app/bin/"
],
"sources": [
    "sources.json",
    "..."
]
```

### Publishing self contained app and trimmed binaries
.NET 9 gives option to include runtime in published application and trim their binaries. This allows you to significantly reduce the size of the package and get rid of `/usr/lib/sdk/dotnet9/bin/install.sh`. 

First you need to have following lines in `sources.json`. These packages are needed to build a project for specific runtime. 

```json
{
    "type": "file",
    "url": "https://api.nuget.org/v3-flatcontainer/microsoft.aspnetcore.app.runtime.linux-arm64/9.0.0/microsoft.aspnetcore.app.runtime.linux-arm64.9.0.0.nupkg",
    "sha512": "0f241403eef87387e31a0a86a539d75e44f9af4dc64a775e7a6dc9ec5d8ef96b0783b9e7f3b2878b62d1f72f112565c70fd71e48e54c06f4cbba533e56f46e3a",
    "dest": "nuget-sources",
    "dest-filename": "microsoft.aspnetcore.app.runtime.linux-arm64.9.0.0.nupkg",
    "x-checker-data": {
        "type": "html",
        "url": "https://dotnetcli.blob.core.windows.net/dotnet/aspnetcore/Runtime/9.0/latest.version",
        "version-pattern": "^([\\d\\.a-z-]+)$",
        "url-template": "https://api.nuget.org/v3-flatcontainer/microsoft.aspnetcore.app.runtime.linux-arm64/$version/microsoft.aspnetcore.app.runtime.linux-arm64.$version.nupkg"
    }
},
{
    "type": "file",
    "url": "https://api.nuget.org/v3-flatcontainer/microsoft.aspnetcore.app.runtime.linux-x64/9.0.0/microsoft.aspnetcore.app.runtime.linux-x64.9.0.0.nupkg",
    "sha512": "6a1d62af51047864ac8630242a9f257dd978e163985c566673276f3919d022cdc878a0a4c2141364d92064ec22793d4db460744cb6dcd21d45495eac511967b9",
    "dest": "nuget-sources",
    "dest-filename": "microsoft.aspnetcore.app.runtime.linux-x64.9.0.0.nupkg",
    "x-checker-data": {
        "type": "html",
        "url": "https://dotnetcli.blob.core.windows.net/dotnet/aspnetcore/Runtime/9.0/latest.version",
        "version-pattern": "^([\\d\\.a-z-]+)$",
        "url-template": "https://api.nuget.org/v3-flatcontainer/microsoft.aspnetcore.app.runtime.linux-x64/$version/microsoft.aspnetcore.app.runtime.linux-x64.$version.nupkg"
    }
},
{
    "type": "file",
    "url": "https://api.nuget.org/v3-flatcontainer/microsoft.netcore.app.runtime.linux-arm64/9.0.0/microsoft.netcore.app.runtime.linux-arm64.9.0.0.nupkg",
    "sha512": "d2ffd83fed2192bef2cefcc62a13734bdff00249d0b47eae1fd934e0d7c8a798a70a3abd2f15ac2fe4b860220d0f0557d6855e512fff015bf64f8b03ba12338e",
    "dest": "nuget-sources",
    "dest-filename": "microsoft.netcore.app.runtime.linux-arm64.9.0.0.nupkg",
    "x-checker-data": {
        "type": "html",
        "url": "https://dotnetcli.blob.core.windows.net/dotnet/aspnetcore/Runtime/9.0/latest.version",
        "version-pattern": "^([\\d\\.a-z-]+)$",
        "url-template": "https://api.nuget.org/v3-flatcontainer/microsoft.netcore.app.runtime.linux-arm64/$version/microsoft.netcore.app.runtime.linux-arm64.$version.nupkg"
    }
},
{
    "type": "file",
    "url": "https://api.nuget.org/v3-flatcontainer/microsoft.netcore.app.runtime.linux-x64/9.0.0/microsoft.netcore.app.runtime.linux-x64.9.0.0.nupkg",
    "sha512": "b53da3f97f2c6899fd27ede233a328270bf99040215b2bb03de6598a9ab6eba603225c04696d850e3a160892552c2def08389d1e59d34fa20a52ffcb30a2a958",
    "dest": "nuget-sources",
    "dest-filename": "microsoft.netcore.app.runtime.linux-x64.9.0.0.nupkg",
    "x-checker-data": {
        "type": "html",
        "url": "https://dotnetcli.blob.core.windows.net/dotnet/aspnetcore/Runtime/9.0/latest.version",
        "version-pattern": "^([\\d\\.a-z-]+)$",
        "url-template": "https://api.nuget.org/v3-flatcontainer/microsoft.netcore.app.runtime.linux-x64/$version/microsoft.netcore.app.runtime.linux-x64.$version.nupkg"
    }
},
```

Then add build options:

```json
"build-options": {
    "arch": {
        "aarch64": {
            "env" : {
                "RUNTIME": "linux-arm64"
            }
        },
        "x86_64": {
            "env" : {
                "RUNTIME": "linux-x64"
            }
        }
    }
},
"build-commands": [
    "mkdir -p /app/bin",
    "dotnet publish -c Release --source ./nuget-sources YourProject.csproj --runtime $RUNTIME --self-contained true",
    "cp -r --remove-destination /run/build/YourProject/bin/Release/net9.0/$RUNTIME/publish/* /app/bin/",
],
```
