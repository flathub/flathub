# .NET 8 SDK extension

## How to use
You need to add following lines to flatpak manifest:

```json
"sdk-extensions": [
    "org.freedesktop.Sdk.Extension.dotnet8"
],
"build-options": {
    "append-path": "/usr/lib/sdk/dotnet8/bin",
    "append-ld-library-path": "/usr/lib/sdk/dotnet8/lib",
    "env": {
        "PKG_CONFIG_PATH": "/app/lib/pkgconfig:/app/share/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig:/usr/lib/sdk/dotnet8/lib/pkgconfig"
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
    "cp -r --remove-destination /run/build/YourProject/bin/Release/net8.0/publish/ /app/bin/",
]
```

### Using nuget packages
If you want to use nuget packages it is recommended to use the [Flatpak .NET Generator](https://github.com/flatpak/flatpak-builder-tools/tree/master/dotnet) tool. It generates sources file that can be included in manifest.

```json
"build-commands": [
    "install.sh",
    "dotnet publish -c Release --source ./nuget-sources YourProject.csproj",
    "cp -r --remove-destination /run/build/YourProject/bin/Release/net8.0/publish/ /app/bin/"
],
"sources": [
    "sources.json",
    "..."
]
```

### Publishing self contained app and trimmed binaries
.NET 8 gives option to include runtime in published application and trim their binaries. This allows you to significantly reduce the size of the package and get rid of `/usr/lib/sdk/dotnet8/bin/install.sh`. 

First you need to have following lines in `sources.json`. These packages are needed to build a project for specific runtime. 

```json
{
    "type": "file",
    "url": "https://api.nuget.org/v3-flatcontainer/microsoft.aspnetcore.app.runtime.linux-arm/8.0.0-rc.1.23421.29/microsoft.aspnetcore.app.runtime.linux-arm.8.0.0-rc.1.23421.29.nupkg",
    "sha512": "ca3bcb64baf61318e71506571fa047b5dda231cbeb45345cbe9e823dc8acfecedc2a9ede51a5b7e03cfa61c3acce24f54f7a44750592ec1d85be89ad34a67d5b",
    "dest": "nuget-sources",
    "dest-filename": "microsoft.aspnetcore.app.runtime.linux-arm.8.0.0.nupkg"
},
{
    "type": "file",
    "url": "https://api.nuget.org/v3-flatcontainer/microsoft.aspnetcore.app.runtime.linux-arm64/8.0.0-rc.1.23421.29/microsoft.aspnetcore.app.runtime.linux-arm64.8.0.0-rc.1.23421.29.nupkg",
    "sha512": "d802e547b4e495f7a6a720857b744e7b78d7d2626e3331935ba90e6c94aba86b8ed122c18306762ff8a4c5bf6739bfbdd332e0dffc58a9efa97336a88e51f2c3",
    "dest": "nuget-sources",
    "dest-filename": "microsoft.aspnetcore.app.runtime.linux-arm64.8.0.0.nupkg"
},
{
    "type": "file",
    "url": "https://api.nuget.org/v3-flatcontainer/microsoft.aspnetcore.app.runtime.linux-x64/8.0.0-rc.1.23421.29/microsoft.aspnetcore.app.runtime.linux-x64.8.0.0-rc.1.23421.29.nupkg",
    "sha512": "d7c5380b6d1d1ff1d0370a9975cbba6ec08e2ec601d1d360b7d5aee9e5dfcfc7ed0a480a6c56ed76a3f4f83f2cac22eed6f9d897d3ac332e3dc4c96285ff21c9",
    "dest": "nuget-sources",
    "dest-filename": "microsoft.aspnetcore.app.runtime.linux-x64.8.0.0.nupkg"
},
{
    "type": "file",
    "url": "https://api.nuget.org/v3-flatcontainer/microsoft.netcore.app.runtime.linux-arm/8.0.0-rc.1.23421.29/microsoft.netcore.app.runtime.linux-arm.8.0.0-rc.1.23421.29.nupkg",
    "sha512": "f97e971886b99a3aea238e299c57e27096c4d1f6c666f0b029606c94ae5118a08b45ae1121561acd9c698f76153dff27c7ae845aa2226ced7ccd58ffe011ae17",
    "dest": "nuget-sources",
    "dest-filename": "microsoft.netcore.app.runtime.linux-arm.8.0.0.nupkg"
},
{
    "type": "file",
    "url": "https://api.nuget.org/v3-flatcontainer/microsoft.netcore.app.runtime.linux-arm64/8.0.0-rc.1.23421.29/microsoft.netcore.app.runtime.linux-arm64.8.0.0-rc.1.23421.29.nupkg",
    "sha512": "34f9ab67ea990387353fb27160d35d0befc0fbeec07e6d36d881cebcd3c83031a480794f712722d41f8226532ad841a9519e3b42ead2c8a41cac2e8c2329a5a6",
    "dest": "nuget-sources",
    "dest-filename": "microsoft.netcore.app.runtime.linux-arm64.8.0.0.nupkg"
},
{
    "type": "file",
    "url": "https://api.nuget.org/v3-flatcontainer/microsoft.netcore.app.runtime.linux-x64/8.0.0-rc.1.23421.29/microsoft.netcore.app.runtime.linux-x64.8.0.0-rc.1.23421.29.nupkg",
    "sha512": "d5c8ad7c7c91182e5e0d10751a8dfa1ff8d998eed6cbe60eaae381b12579c7f921c3106a02206b0f1be756990872f81a3864fcda134a001583b3125bdb02389d",
    "dest": "nuget-sources",
    "dest-filename": "microsoft.netcore.app.runtime.linux-x64.8.0.0.nupkg"
},
```

Then add build options:

```json
"build-options": {
    "arch": {
        "arm": {
            "env" : {
                "RUNTIME": "linux-arm"
            }
        },
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
    "cp -r --remove-destination /run/build/YourProject/bin/Release/net8.0/$RUNTIME/publish/* /app/bin/",
],
```
