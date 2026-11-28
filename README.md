# .NET 11 SDK extension

## How to use
You need to add following lines to flatpak manifest:

```json
"sdk-extensions": [
    "org.freedesktop.Sdk.Extension.dotnet11"
],
"build-options": {
    "append-path": "/usr/lib/sdk/dotnet11/bin",
    "append-ld-library-path": "/usr/lib/sdk/dotnet11/lib",
    "prepend-pkg-config-path": "/usr/lib/sdk/dotnet11/lib/pkgconfig"
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
    "cp -r --remove-destination /run/build/YourProject/bin/Release/net11.0/publish/ /app/bin/",
]
```

### Using nuget packages
If you want to use nuget packages it is recommended to use the [Flatpak .NET Generator](https://github.com/flatpak/flatpak-builder-tools/tree/master/dotnet) tool. It generates sources file that can be included in manifest.

An alternative tool is [Nickvision.FlatpakGenerator](https://www.nuget.org/packages/Nickvision.FlatpakGenerator/), a native .NET tool, that will also generate the same sources file. This tool additionally contains some more options compared to the above tool. For example, the ability to specify the .NET and FreeDesktop SDK versions and to generate a sources file for self-contained .NET apps. 

```json
"build-commands": [
    "install.sh",
    "dotnet publish -c Release --source ./nuget-sources --source /usr/lib/sdk/dotnet11/nuget/packages YourProject.csproj",
    "cp -r --remove-destination /run/build/YourProject/bin/Release/net11.0/publish/ /app/bin/"
],
"sources": [
    "sources.json",
    "..."
]
```

### Publishing self contained app and trimmed binaries
.NET 11 gives option to include runtime in published application and trim their binaries. This allows you to significantly reduce the size of the package and get rid of `/usr/lib/sdk/dotnet11/bin/install.sh`. 

Slightly modify the `build-commands` and add the `build-options` as follows:

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
    "dotnet publish -c Release --source ./nuget-sources --source /usr/lib/sdk/dotnet11/nuget/packages YourProject.csproj --runtime $RUNTIME --self-contained true",
    "cp -r --remove-destination /run/build/YourProject/bin/Release/net11.0/$RUNTIME/publish/* /app/bin/",
],
```

Note that your nuget packages directory is listed before the one provided with the SDK, the build may fail if ordered differently.
